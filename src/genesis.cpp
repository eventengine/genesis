#include "genesis.hpp"
#include "audio_file.hpp"
#include "threads.hpp"

static atomic_flag init_flag = ATOMIC_FLAG_INIT;

struct GenesisPortDescriptor {
    enum GenesisPortType port_type;
    char *name;
};

struct GenesisNotesPortDescriptor {
    struct GenesisPortDescriptor port_descriptor;
};

struct GenesisParamPortDescriptor {
    struct GenesisPortDescriptor port_descriptor;
};

struct GenesisAudioPortDescriptor {
    struct GenesisPortDescriptor port_descriptor;

    bool channel_layout_fixed;
    // if channel_layout_fixed is true then this is the index
    // of the other port that it is the same as, or -1 if it is fixed
    // to the value of channel_layout
    int same_channel_layout_index;
    struct GenesisChannelLayout channel_layout;

    bool sample_rate_fixed;
    // if sample_rate_fixed is true then this is the index
    // of the other port that it is the same as, or -1 if it is fixed
    // to the value of sample_rate
    int same_sample_rate_index;
    int sample_rate;
};

struct GenesisNodeDescriptor {
    struct GenesisContext *context;
    char *name;
    char *description;
    List<GenesisPortDescriptor*> port_descriptors;
    void (*run)(struct GenesisNode *node);
};

struct GenesisPort {
    struct GenesisPortDescriptor *descriptor;
    struct GenesisNode *node;
    struct GenesisPort *input_from;
    struct GenesisPort *output_to;
};

struct GenesisAudioPort {
    struct GenesisPort port;
    struct GenesisChannelLayout channel_layout;
    int sample_rate;
};

struct GenesisParamPort {
    struct GenesisPort port;
};

struct GenesisNotesPort {
    struct GenesisPort port;
};

struct GenesisNode {
    const struct GenesisNodeDescriptor *descriptor;
    int port_count;
    struct GenesisPort **ports;
    int set_index; // index into context->nodes
    atomic_flag being_processed;
};

static void on_midi_devices_change(MidiHardware *midi_hardware) {
    GenesisContext *context = reinterpret_cast<GenesisContext *>(midi_hardware->userdata);
    if (context->midi_change_callback)
        context->midi_change_callback(context->midi_change_callback_userdata);
}

static void midi_events_signal(MidiHardware *midi_hardware) {
    GenesisContext *context = reinterpret_cast<GenesisContext *>(midi_hardware->userdata);
    genesis_wakeup(context);
}

static void on_devices_change(AudioHardware *audio_hardware) {
    GenesisContext *context = reinterpret_cast<GenesisContext *>(audio_hardware->_userdata);
    if (context->devices_change_callback)
        context->devices_change_callback(context->devices_change_callback_userdata);
}

static void on_audio_hardware_events_signal(AudioHardware *audio_hardware) {
    GenesisContext *context = reinterpret_cast<GenesisContext *>(audio_hardware->_userdata);
    genesis_wakeup(context);
}

int genesis_create_context(struct GenesisContext **out_context) {
    *out_context = nullptr;

    // only call global initialization once
    if (!init_flag.test_and_set()) {
        audio_file_init();
    }

    GenesisContext *context = create_zero<GenesisContext>();
    if (!context) {
        genesis_destroy_context(context);
        return GenesisErrorNoMem;
    }

    if (context->events_cond.error() || context->events_mutex.error()) {
        genesis_destroy_context(context);
        return context->events_cond.error() || context->events_mutex.error();
    }

    if (context->task_cond.error() || context->task_mutex.error()) {
        genesis_destroy_context(context);
        return context->task_cond.error() || context->task_mutex.error();
    }

    if (context->thread_pool.resize(Thread::concurrency())) {
        genesis_destroy_context(context);
        return GenesisErrorNoMem;
    }

    int err = create_midi_hardware(context, "genesis", midi_events_signal, on_midi_devices_change,
            context, &context->midi_hardware);
    if (err) {
        genesis_destroy_context(context);
        return err;
    }

    err = audio_file_get_out_formats(context->out_formats);
    if (err) {
        genesis_destroy_context(context);
        return err;
    }

    err = audio_file_get_in_formats(context->in_formats);
    if (err) {
        genesis_destroy_context(context);
        return err;
    }

    context->audio_hardware.set_on_devices_change(on_devices_change);
    context->audio_hardware.set_on_events_signal(on_audio_hardware_events_signal);
    context->audio_hardware._userdata = context;

    GenesisNodeDescriptor *node_descr = create_zero<GenesisNodeDescriptor>();
    if (!node_descr) {
        genesis_destroy_context(context);
        return GenesisErrorNoMem;
    }

    err = context->node_descriptors.append(node_descr);
    if (err) {
        genesis_destroy_context(context);
        return err;
    }

    node_descr->name = strdup("synth");
    node_descr->description = strdup("A single oscillator.");

    if (!node_descr->name || !node_descr->description) {
        genesis_destroy_context(context);
        return GenesisErrorNoMem;
    }

    GenesisNotesPortDescriptor *notes_port = create<GenesisNotesPortDescriptor>();
    GenesisAudioPortDescriptor *audio_port = create<GenesisAudioPortDescriptor>();

    err = node_descr->port_descriptors.append(&notes_port->port_descriptor);
    if (err) {
        genesis_destroy_context(context);
        return err;
    }

    err = node_descr->port_descriptors.append(&audio_port->port_descriptor);
    if (err) {
        genesis_destroy_context(context);
        return err;
    }

    notes_port->port_descriptor.port_type = GenesisPortTypeNotesIn;
    notes_port->port_descriptor.name = strdup("notes_in");
    if (!notes_port->port_descriptor.name) {
        genesis_destroy_context(context);
        return GenesisErrorNoMem;
    }

    audio_port->port_descriptor.port_type = GenesisPortTypeAudioOut;
    audio_port->port_descriptor.name = strdup("audio_out");
    if (!notes_port->port_descriptor.name) {
        genesis_destroy_context(context);
        return GenesisErrorNoMem;
    }
    audio_port->channel_layout =
        *genesis_channel_layout_get_builtin(GenesisChannelLayoutIdMono);
    audio_port->channel_layout_fixed = false;
    audio_port->same_channel_layout_index = -1;
    audio_port->sample_rate = 48000;
    audio_port->sample_rate_fixed = false;
    audio_port->same_sample_rate_index = -1;

    *out_context = context;
    return 0;
}

void genesis_destroy_context(struct GenesisContext *context) {
    if (context) {
        if (context->midi_hardware) {
            destroy_midi_hardware(context->midi_hardware);
        }
        for (int i = 0; i < context->node_descriptors.length(); i += 1) {
            destroy(context->node_descriptors.at(i), 1);
        }
        for (int i = 0; i < context->out_formats.length(); i += 1) {
            destroy(context->out_formats.at(i), 1);
        }
        for (int i = 0; i < context->in_formats.length(); i += 1) {
            destroy(context->in_formats.at(i), 1);
        }
        destroy(context, 1);
    }
}

void genesis_flush_events(struct GenesisContext *context) {
    context->audio_hardware.flush_events();
    midi_hardware_flush_events(context->midi_hardware);
}

void genesis_wait_events(struct GenesisContext *context) {
    genesis_flush_events(context);
    context->events_mutex.lock();
    context->events_cond.wait(&context->events_mutex);
    context->events_mutex.unlock();
}

void genesis_wakeup(struct GenesisContext *context) {
    context->events_mutex.lock();
    context->events_cond.signal();
    context->events_mutex.unlock();
}

struct GenesisNodeDescriptor *genesis_node_descriptor_find(
        GenesisContext *context, const char *name)
{
    for (int i = 0; i < context->node_descriptors.length(); i += 1) {
        GenesisNodeDescriptor *descr = context->node_descriptors.at(i);
        if (strcmp(descr->name, name) == 0)
            return descr;
    }
    return nullptr;
}

const char *genesis_node_descriptor_name(const struct GenesisNodeDescriptor *node_descriptor) {
    return node_descriptor->name;
}

const char *genesis_node_descriptor_description(const struct GenesisNodeDescriptor *node_descriptor) {
    return node_descriptor->description;
}

static GenesisAudioPort *create_audio_port_from_descriptor(GenesisPortDescriptor *port_descriptor) {
    GenesisAudioPort *audio_port = create_zero<GenesisAudioPort>();
    if (!audio_port)
        return nullptr;
    audio_port->port.descriptor = port_descriptor;
    return audio_port;
}

static GenesisNotesPort *create_notes_port_from_descriptor(GenesisPortDescriptor *port_descriptor) {
    GenesisNotesPort *notes_port = create_zero<GenesisNotesPort>();
    if (!notes_port)
        return nullptr;
    notes_port->port.descriptor = port_descriptor;
    return notes_port;
}

static GenesisParamPort *create_param_port_from_descriptor(GenesisPortDescriptor *port_descriptor) {
    GenesisParamPort *param_port = create_zero<GenesisParamPort>();
    if (!param_port)
        return nullptr;
    param_port->port.descriptor = port_descriptor;
    return param_port;
}

static GenesisPort *create_port_from_descriptor(GenesisPortDescriptor *port_descriptor) {
    switch (port_descriptor->port_type) {
        case GenesisPortTypeAudioIn:
        case GenesisPortTypeAudioOut:
            return &create_audio_port_from_descriptor(port_descriptor)->port;

        case GenesisPortTypeNotesIn:
        case GenesisPortTypeNotesOut:
            return &create_notes_port_from_descriptor(port_descriptor)->port;

        case GenesisPortTypeParamIn:
        case GenesisPortTypeParamOut:
            return &create_param_port_from_descriptor(port_descriptor)->port;
    }
    panic("invalid port type");
}

struct GenesisNode *genesis_node_descriptor_create_node(struct GenesisNodeDescriptor *node_descriptor) {
    GenesisNode *node = allocate_zero<GenesisNode>(1);
    if (!node) {
        genesis_node_destroy(node);
        return nullptr;
    }
    node->descriptor = node_descriptor;
    node->port_count = node_descriptor->port_descriptors.length();
    node->ports = allocate_zero<GenesisPort*>(node->port_count);
    if (!node->ports) {
        genesis_node_destroy(node);
        return nullptr;
    }
    for (int i = 0; i < node->port_count; i += 1) {
        GenesisPort *port = create_port_from_descriptor(node_descriptor->port_descriptors.at(i));
        if (!port) {
            genesis_node_destroy(node);
            return nullptr;
        }
        node->ports[i] = port;
    }
    node->set_index = context->nodes.length();
    if (context->nodes.append(node)) {
        genesis_node_destroy(node);
        return nullptr;
    }
    return node;
}

void genesis_node_destroy(struct GenesisNode *node) {
    if (node) {
        context->nodes.swap_remove(node->set_index);
        if (node->set_index < context->nodes.length())
            context->nodes.at(node->set_index)->set_index = node->set_index;

        if (node->ports) {
            for (int i = 0; i < node->port_count; i += 1) {
                if (node->ports[i]) {
                    destroy(node->ports[i], 1);
                }
            }
            destroy(node->ports, 1);
        }

        destroy(node, 1);
    }
}

void genesis_refresh_audio_devices(struct GenesisContext *context) {
    context->audio_hardware.block_until_ready();
    context->audio_hardware.flush_events();
    context->audio_hardware.block_until_have_devices();
}

int genesis_get_audio_device_count(struct GenesisContext *context) {
    const AudioDevicesInfo *audio_device_info = context->audio_hardware.devices_info();
    if (!audio_device_info)
        return -1;
    return audio_device_info->devices.length();
}

struct GenesisAudioDevice *genesis_get_audio_device(struct GenesisContext *context, int index) {
    const AudioDevicesInfo *audio_device_info = context->audio_hardware.devices_info();
    if (!audio_device_info)
        return nullptr;
    if (index < 0 || index >= audio_device_info->devices.length())
        return nullptr;
    return (GenesisAudioDevice *) &audio_device_info->devices.at(index);
}

int genesis_get_default_playback_device_index(struct GenesisContext *context) {
    const AudioDevicesInfo *audio_device_info = context->audio_hardware.devices_info();
    if (!audio_device_info)
        return -1;
    return audio_device_info->default_output_index;
}

int genesis_get_default_recording_device_index(struct GenesisContext *context) {
    const AudioDevicesInfo *audio_device_info = context->audio_hardware.devices_info();
    if (!audio_device_info)
        return -1;
    return audio_device_info->default_input_index;
}

const char *genesis_audio_device_name(const struct GenesisAudioDevice *audio_device) {
    return audio_device->name.raw();
}

const char *genesis_audio_device_description(const struct GenesisAudioDevice *audio_device) {
    return audio_device->description.raw();
}

enum GenesisAudioDevicePurpose genesis_audio_device_purpose(const struct GenesisAudioDevice *audio_device) {
    return audio_device->purpose;
}

void genesis_set_audio_device_callback(struct GenesisContext *context,
        void (*callback)(void *userdata),
        void *userdata)
{
    context->devices_change_callback_userdata = userdata;
    context->devices_change_callback = callback;
}

int genesis_in_format_count(struct GenesisContext *context) {
    return context->in_formats.length();
}

int genesis_out_format_count(struct GenesisContext *context) {
    return context->out_formats.length();
}

struct GenesisAudioFileFormat *genesis_in_format_index(
        struct GenesisContext *context, int format_index)
{
    return context->in_formats.at(format_index);
}

struct GenesisAudioFileFormat *genesis_out_format_index(
        struct GenesisContext *context, int format_index)
{
    return context->out_formats.at(format_index);
}

struct GenesisAudioFileCodec *genesis_guess_audio_file_codec(
        struct GenesisContext *context, const char *filename_hint,
        const char *format_name, const char *codec_name)
{
    return audio_file_guess_audio_file_codec(context->out_formats, filename_hint, format_name, codec_name);
}

int genesis_audio_file_codec_sample_format_count(const struct GenesisAudioFileCodec *codec) {
    return codec ? codec->prioritized_sample_formats.length() : 0;
}

enum GenesisSampleFormat genesis_audio_file_codec_sample_format_index(
        const struct GenesisAudioFileCodec *codec, int index)
{
    return codec->prioritized_sample_formats.at(index);
}

int genesis_audio_file_codec_sample_rate_count(const struct GenesisAudioFileCodec *codec) {
    return codec ? codec->prioritized_sample_rates.length() : 0;
}

int genesis_audio_file_codec_sample_rate_index(
        const struct GenesisAudioFileCodec *codec, int index)
{
    return codec->prioritized_sample_rates.at(index);
}

int genesis_audio_device_create_node_descriptor(struct GenesisAudioDevice *audio_device,
        struct GenesisNodeDescriptor **out)
{
    GenesisContext *context = audio_device->context;

    *out = nullptr;

    if (audio_device->purpose != GenesisAudioDevicePurposePlayback) {
        return GenesisErrorInvalidState;
    }

    GenesisNodeDescriptor *node_descr = genesis_create_node_descriptor(context, 1);

    if (!node_descr) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    node_descr->name = create_formatted_str("playback-device-%s", audio_device->name.raw());
    node_descr->description = create_formatted_str("Playback Device: %s", audio_device->description.raw());

    if (!node_descr->name || !node_descr->description) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    GenesisAudioPortDescriptor *audio_in_port = create_zero<GenesisAudioPortDescriptor>();
    if (!audio_in_port) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    node_descr->port_descriptors.at(0) = &audio_in_port->port_descriptor;

    audio_in_port->port_descriptor.name = strdup("audio_in");
    if (!audio_in_port->port_descriptor.name) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    audio_in_port->port_descriptor.port_type = GenesisPortTypeAudioIn;
    audio_in_port->channel_layout = audio_device->channel_layout;
    audio_in_port->channel_layout_fixed = true;
    audio_in_port->same_channel_layout_index = -1;
    audio_in_port->sample_rate = audio_device->default_sample_rate;
    audio_in_port->sample_rate_fixed = true;
    audio_in_port->same_sample_rate_index = -1;

    *out = node_descr;
    return 0;
}

int genesis_midi_device_create_node_descriptor(
        struct GenesisMidiDevice *midi_device,
        struct GenesisNodeDescriptor **out)
{
    GenesisContext *context = midi_device->midi_hardware->context;
    *out = nullptr;

    GenesisNodeDescriptor *node_descr = genesis_create_node_descriptor(context, 2);

    if (!node_descr) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    node_descr->name = create_formatted_str("midi-device-%s", genesis_midi_device_name(midi_device));
    node_descr->description = create_formatted_str("Midi Device: %s", genesis_midi_device_description(midi_device));

    if (!node_descr->name || !node_descr->description) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    GenesisNotesPortDescriptor *notes_out_port = create_zero<GenesisNotesPortDescriptor>();
    if (!notes_out_port) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    node_descr->port_descriptors.at(0) = &notes_out_port->port_descriptor;

    notes_out_port->port_descriptor.name = strdup("notes_out");
    if (!notes_out_port->port_descriptor.name) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }
    notes_out_port->port_descriptor.port_type = GenesisPortTypeNotesOut;


    GenesisParamPortDescriptor *param_out_port = create_zero<GenesisParamPortDescriptor>();
    if (!param_out_port) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }

    node_descr->port_descriptors.at(1) = &param_out_port->port_descriptor;

    param_out_port->port_descriptor.name = strdup("param_out");
    if (!param_out_port->port_descriptor.name) {
        genesis_node_descriptor_destroy(node_descr);
        return GenesisErrorNoMem;
    }
    param_out_port->port_descriptor.port_type = GenesisPortTypeParamOut;

    *out = node_descr;
    return 0;
}

static void destroy_audio_port_descriptor(GenesisAudioPortDescriptor *audio_port_descr) {
    destroy(audio_port_descr, 1);
}

static void destroy_notes_port_descriptor(GenesisNotesPortDescriptor *notes_port_descr) {
    destroy(notes_port_descr, 1);
}

static void destroy_param_port_descriptor(GenesisParamPortDescriptor *param_port_descr) {
    destroy(param_port_descr, 1);
}

void genesis_port_descriptor_destroy(struct GenesisPortDescriptor *port_descriptor) {
    switch (port_descriptor->port_type) {
    case GenesisPortTypeAudioIn:
    case GenesisPortTypeAudioOut:
        destroy_audio_port_descriptor((GenesisAudioPortDescriptor *)port_descriptor);
        break;
    case GenesisPortTypeNotesIn:
    case GenesisPortTypeNotesOut:
        destroy_notes_port_descriptor((GenesisNotesPortDescriptor *)port_descriptor);
        break;
    case GenesisPortTypeParamIn:
    case GenesisPortTypeParamOut:
        destroy_param_port_descriptor((GenesisParamPortDescriptor *)port_descriptor);
        break;
    }
    free(port_descriptor->name);
}

void genesis_node_descriptor_destroy(struct GenesisNodeDescriptor *node_descriptor) {
    if (node_descriptor) {
        for (int i = 0; i < node_descriptor->port_descriptors.length(); i += 1) {
            GenesisPortDescriptor *port_descriptor = node_descriptor->port_descriptors.at(i);
            genesis_port_descriptor_destroy(port_descriptor);
        }

        free(node_descriptor->name);
        free(node_descriptor->description);

        destroy(node_descriptor, 1);
    }
}

static void resolve_channel_layout(GenesisAudioPort *audio_port) {
    GenesisAudioPortDescriptor *port_descr = (GenesisAudioPortDescriptor*)audio_port->port.descriptor;
    if (port_descr->channel_layout_fixed) {
        if (port_descr->same_channel_layout_index >= 0) {
            GenesisAudioPort *other_port = (GenesisAudioPort *)
                &audio_port->port.node->ports[port_descr->same_channel_layout_index];
            audio_port->channel_layout = other_port->channel_layout;
        } else {
            audio_port->channel_layout = port_descr->channel_layout;
        }
    }
}

static void resolve_sample_rate(GenesisAudioPort *audio_port) {
    GenesisAudioPortDescriptor *port_descr = (GenesisAudioPortDescriptor *)audio_port->port.descriptor;
    if (port_descr->sample_rate_fixed) {
        if (port_descr->same_sample_rate_index >= 0) {
            GenesisAudioPort *other_port = (GenesisAudioPort *)
                &audio_port->port.node->ports[port_descr->same_sample_rate_index];
            audio_port->sample_rate = other_port->sample_rate;
        } else {
            audio_port->sample_rate = port_descr->sample_rate;
        }
    }
}

static int connect_audio_ports(GenesisAudioPort *source, GenesisAudioPort *dest) {
    GenesisAudioPortDescriptor *source_audio_descr = (GenesisAudioPortDescriptor *) source->port.descriptor;
    GenesisAudioPortDescriptor *dest_audio_descr = (GenesisAudioPortDescriptor *) dest->port.descriptor;

    resolve_channel_layout(source);
    resolve_channel_layout(dest);
    if (source_audio_descr->channel_layout_fixed &&
        dest_audio_descr->channel_layout_fixed)
    {
        // both fixed. they better match up
        if (!genesis_channel_layout_equal(&source_audio_descr->channel_layout,
                    &dest_audio_descr->channel_layout))
        {
            return GenesisErrorIncompatibleChannelLayouts;
        }
    } else if (!source_audio_descr->channel_layout_fixed &&
               !dest_audio_descr->channel_layout_fixed)
    {
        // anything goes. default to mono
        source->channel_layout = *genesis_channel_layout_get_builtin(GenesisChannelLayoutIdMono);
        dest->channel_layout = source->channel_layout;
    } else if (source_audio_descr->channel_layout_fixed) {
        // source is fixed, use that one
        dest->channel_layout = source->channel_layout;
    } else {
        // dest is fixed, use that one
        source->channel_layout = dest->channel_layout;
    }

    resolve_sample_rate(source);
    resolve_sample_rate(dest);
    if (source_audio_descr->sample_rate_fixed && dest_audio_descr->sample_rate_fixed) {
        // both fixed. they better match up
        if (source_audio_descr->sample_rate != dest_audio_descr->sample_rate)
            return GenesisErrorIncompatibleSampleRates;
    } else if (!source_audio_descr->sample_rate_fixed &&
               !dest_audio_descr->sample_rate_fixed)
    {
        // anything goes. default to 48,000 Hz
        source->sample_rate = 48000;
        dest->sample_rate = source->sample_rate;
    } else if (source_audio_descr->sample_rate_fixed) {
        // source is fixed, use that one
        dest->sample_rate = source->sample_rate;
    } else {
        // dest is fixed, use that one
        source->sample_rate = dest->sample_rate;
    }

    source->port.output_to = &dest->port;
    dest->port.input_from = &source->port;
    return 0;
}

static int connect_notes_ports(GenesisNotesPort *source, GenesisNotesPort *dest) {
    source->port.output_to = &dest->port;
    dest->port.input_from = &source->port;
    return 0;
}

int genesis_connect_ports(struct GenesisPort *source, struct GenesisPort *dest) {
    // perform validation
    switch (source->descriptor->port_type) {
        case GenesisPortTypeAudioOut:
            if (dest->descriptor->port_type != GenesisPortTypeAudioIn)
                return GenesisErrorIncompatiblePorts;

            return connect_audio_ports((GenesisAudioPort *)source, (GenesisAudioPort *)dest);
        case GenesisPortTypeNotesOut:
            if (dest->descriptor->port_type != GenesisPortTypeNotesIn)
                return GenesisErrorIncompatiblePorts;

            return connect_notes_ports((GenesisNotesPort *)source, (GenesisNotesPort *)dest);
        case GenesisPortTypeParamOut:
            if (dest->descriptor->port_type != GenesisPortTypeParamIn)
                return GenesisErrorIncompatiblePorts;
            panic("unimplemented: connect param ports");
        case GenesisPortTypeAudioIn:
        case GenesisPortTypeNotesIn:
        case GenesisPortTypeParamIn:
            return GenesisErrorInvalidPortDirection;
    }
    panic("unknown port type");
}

int genesis_node_descriptor_find_port_index(
        const struct GenesisNodeDescriptor *node_descriptor, const char *name)
{
    for (int i = 0; i < node_descriptor->port_descriptors.length(); i += 1) {
        GenesisPortDescriptor *port_descriptor = node_descriptor->port_descriptors.at(i);
        if (strcmp(port_descriptor->name, name) == 0)
            return i;
    }
    return -1;
}

struct GenesisPort *genesis_node_port(struct GenesisNode *node, int port_index) {
    if (port_index < 0 || port_index >= node->port_count)
        return nullptr;

    return node->ports[port_index];
}

struct GenesisNode *genesis_port_node(struct GenesisPort *port) {
    return port->node;
}

struct GenesisNodeDescriptor *genesis_create_node_descriptor(
        struct GenesisContext *context, int port_count)
{
    GenesisNodeDescriptor *node_descr = create_zero<GenesisNodeDescriptor>();
    if (!node_descr)
        return nullptr;

    if (node_descr->port_descriptors.resize(port_count)) {
        genesis_node_descriptor_destroy(node_descr);
        return nullptr;
    }

    if (context->node_descriptors.append(node_descr)) {
        genesis_node_descriptor_destroy(node_descr);
        return nullptr;
    }

    return node_descr;
}

static GenesisAudioPortDescriptor *create_audio_port(GenesisNodeDescriptor *node_descr,
        int port_index, GenesisPortType port_type)
{
    GenesisAudioPortDescriptor *audio_port_descr = create_zero<GenesisAudioPortDescriptor>();
    node_descr->port_descriptors.at(port_index) = &audio_port_descr->port_descriptor;
    audio_port_descr->port_descriptor.port_type = port_type;
    return audio_port_descr;
}

static GenesisNotesPortDescriptor *create_notes_port(GenesisNodeDescriptor *node_descr,
        int port_index, GenesisPortType port_type)
{
    GenesisNotesPortDescriptor *notes_port_descr = create_zero<GenesisNotesPortDescriptor>();
    node_descr->port_descriptors.at(port_index) = &notes_port_descr->port_descriptor;
    notes_port_descr->port_descriptor.port_type = port_type;
    return notes_port_descr;
}

static GenesisParamPortDescriptor *create_param_port(GenesisNodeDescriptor *node_descr,
        int port_index, GenesisPortType port_type)
{
    GenesisParamPortDescriptor *param_port_descr = create_zero<GenesisParamPortDescriptor>();
    node_descr->port_descriptors.at(port_index) = &param_port_descr->port_descriptor;
    param_port_descr->port_descriptor.port_type = port_type;
    return param_port_descr;
}

struct GenesisPortDescriptor *genesis_node_descriptor_create_port(
        struct GenesisNodeDescriptor *node_descr, int port_index,
        enum GenesisPortType port_type)
{
    if (port_index < 0 || port_index >= node_descr->port_descriptors.length())
        return nullptr;
    switch (port_type) {
        case GenesisPortTypeAudioIn:
        case GenesisPortTypeAudioOut:
            return &create_audio_port(node_descr, port_index, port_type)->port_descriptor;
        case GenesisPortTypeNotesIn:
        case GenesisPortTypeNotesOut:
            return &create_notes_port(node_descr, port_index, port_type)->port_descriptor;
        case GenesisPortTypeParamIn:
        case GenesisPortTypeParamOut:
            return &create_param_port(node_descr, port_index, port_type)->port_descriptor;
    }
    panic("invalid port type");
}

static void debug_print_audio_port_config(GenesisAudioPort *port) {
    resolve_channel_layout(port);
    resolve_sample_rate(port);

    GenesisAudioPortDescriptor *audio_descr = (GenesisAudioPortDescriptor *)port->port.descriptor;
    const char *chan_layout_fixed = audio_descr->channel_layout_fixed ? "(fixed)" : "(any)";
    const char *sample_rate_fixed = audio_descr->sample_rate_fixed ? "(fixed)" : "(any)";

    fprintf(stderr, "audio port: %s\n", port->port.descriptor->name);
    fprintf(stderr, "sample rate: %s %d\n", sample_rate_fixed, port->sample_rate);
    fprintf(stderr, "channel_layout: %s ", chan_layout_fixed);
    genesis_debug_print_channel_layout(&port->channel_layout);
}

static void debug_print_notes_port_config(GenesisNotesPort *port) {
    fprintf(stderr, "notes port: %s\n", port->port.descriptor->name);
}

static void debug_print_param_port_config(GenesisParamPort *port) {
    fprintf(stderr, "param port: %s\n", port->port.descriptor->name);
}

void genesis_debug_print_port_config(struct GenesisPort *port) {
    switch (port->descriptor->port_type) {
        case GenesisPortTypeAudioIn:
        case GenesisPortTypeAudioOut:
            debug_print_audio_port_config((GenesisAudioPort *)port);
            return;
        case GenesisPortTypeNotesIn:
        case GenesisPortTypeNotesOut:
            debug_print_notes_port_config((GenesisNotesPort *)port);
            return;
        case GenesisPortTypeParamIn:
        case GenesisPortTypeParamOut:
            debug_print_param_port_config((GenesisParamPort *)port);
            return;
    }
    panic("invalid port type");
}

static bool node_all_buffers_full(GenesisNode *node) {
    // TODO
}

static GenesisNode *task_queue_get_next_blocking(GenesisContext *context) {
    // this function must return the next node in the queue without locking.
    // however, if the queue is empty then the function should block until
    // it can return a node.
    // TODO
}

static void task_queue_add(GenesisContext *context, GenesisNode *node) {
    // TODO
}

static void task_queue_wakeup(GenesisContext *context) {
    // TODO
}

static void pipeline_thread_run(void *userdata) {
    GenesisContext *context = reinterpret_cast<GenesisContext*>(userdata);
    for (;;) {
        GenesisNode *node = task_queue_get_next_blocking(context);
        if (context->pipeline_shutdown)
            break;

        const GenesisNodeDescriptor *node_descriptor = node->descriptor;
        node_descriptor->run(node);
        node->being_processed.clear();
        // we do not acquire the mutex here, so this signal is advisory only.
        // it might not wake up the manager thread.
        context->task_cond.signal();
    }
}

static void manager_thread_run(void *userdata) {
    GenesisContext *context = reinterpret_cast<GenesisContext*>(userdata);
    while (!context->pipeline_shutdown) {
        // scan the node list and add tasks for nodes which are not being processed
        // and whose buffers are not full
        for (int node_index = 0; node_index < context->nodes.length(); node_index += 1) {
            GenesisNode *node = context->nodes.at(node_index);
            if (!node->being_processed.test_and_set()) {
                // determine whether we want to process this node
                if (node_all_buffers_full(node)) {
                    node->being_processed.clear();
                    continue;
                }

                // don't add to queue if children are not done
                bool all_children_done = true;
                for (int i = 0; i < node->port_count; i += 1) {
                    GenesisPort *port = node->ports[i];
                    GenesisPort *child_port = port->input_from;
                    if (child_port == port)
                        continue;
                    GenesisNode *child_node = child_port->node;
                    if (child_node->being_processed.test_and_set()) {
                        // this child is currently being processed
                        all_children_done = false;
                        break;
                    } else {
                        child_node->being_processed.clear();
                        if (!node_all_buffers_full(child_node)) {
                            all_children_done = false;
                            break;
                        }
                    }
                }

                if (all_children_done) {
                    task_queue_add(context, node);
                } else {
                    node->being_processed.clear();
                }
            }
        }

        context->task_mutex.lock();
        context->task_cond.wait(&context->task_mutex);
        context->task_mutex.unlock();
    }
}

int genesis_start_pipeline(struct GenesisContext *context) {
    context->task_queue = create_zero<RingBuffer>(context->nodes.length() * sizeof(GenesisNode *));
    if (!context->task_queue) {
        genesis_stop_pipeline(context);
        return GenesisErrorNoMem;
    }

    // initialize nodes
    for (int i = 0; i < context->nodes.length(); i += 1) {
        GenesisNode *node = context->nodes.at(i);
        // TODO clear node buffers
        // TODO set node buffers latency
        node->being_processed.clear();
    }
    context->pipeline_shutdown = false;

    int err = context->manager_thread.start(manager_thread_run, context);
    if (err) {
        genesis_stop_pipeline(context);
        return err;
    }
    for (int i = 0; i < context->thread_pool.length(); i += 1) {
        Thread *thread = &context->thread_pool.at(i);
        int err = thread->start(pipeline_thread_run, context);
        if (err) {
            genesis_stop_pipeline(context);
            return err;
        }
    }

    return 0;
}

void genesis_stop_pipeline(struct GenesisContext *context) {
    context->pipeline_shutdown = true;
    context->task_mutex.lock();
    context->task_cond.signal();
    context->task_mutex.unlock();
    task_queue_wakeup(context);
    if (context->manager_thread.running())
        context->manager_thread.join();
    for (int i = 0; i < context->thread_pool.length(); i += 1) {
        Thread *thread = &context->thread_pool.at(i);
        if (thread->running())
            thread->join();
    }

    destroy(context->task_queue, 1);
    context->task_queue = nullptr;
}
