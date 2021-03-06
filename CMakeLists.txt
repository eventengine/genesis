cmake_minimum_required(VERSION 2.8.5)
project(genesis C CXX)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

if(CMAKE_VERSION VERSION_LESS 3.0.0)
    set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "library install dir (lib)")
    set(CMAKE_INSTALL_INCLUDEDIR "include" CACHE PATH "header base install dir (include)")
    set(CMAKE_INSTALL_BINDIR "bin" CACHE PATH "user executables (bin)")
else()
    cmake_policy(SET CMP0042 NEW)
    cmake_policy(SET CMP0046 NEW)
    include(GNUInstallDirs)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

set(LIBGENESIS_VERSION_MAJOR 0)
set(LIBGENESIS_VERSION_MINOR 0)
set(LIBGENESIS_VERSION_PATCH 0)
set(LIBGENESIS_VERSION "${LIBGENESIS_VERSION_MAJOR}.${LIBGENESIS_VERSION_MINOR}.${LIBGENESIS_VERSION_PATCH}")
message("Configuring libgenesis version ${LIBGENESIS_VERSION}")

set(LIBGENESIS_SOURCES
    "${CMAKE_SOURCE_DIR}/src/audio_file.cpp"
    "${CMAKE_SOURCE_DIR}/src/byte_buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/delay.cpp"
    "${CMAKE_SOURCE_DIR}/src/error.cpp"
    "${CMAKE_SOURCE_DIR}/src/genesis.cpp"
    "${CMAKE_SOURCE_DIR}/src/midi_hardware.cpp"
    "${CMAKE_SOURCE_DIR}/src/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/random.cpp"
    "${CMAKE_SOURCE_DIR}/src/resample.cpp"
    "${CMAKE_SOURCE_DIR}/src/ring_buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/sha_256_hasher.cpp"
    "${CMAKE_SOURCE_DIR}/src/string.cpp"
    "${CMAKE_SOURCE_DIR}/src/synth.cpp"
    "${CMAKE_SOURCE_DIR}/src/util.cpp"
    "${CMAKE_SOURCE_DIR}/src/warning.cpp"
)
set(CONFIGURE_OUT_FILE "${CMAKE_BINARY_DIR}/config.h")
set(LIBGENESIS_HEADERS
    "${CMAKE_SOURCE_DIR}/src/genesis.h"
)

set(GENESIS_SOURCES
    "${CMAKE_SOURCE_DIR}/src/alpha_texture.cpp"
    "${CMAKE_SOURCE_DIR}/src/audio_graph.cpp"
    "${CMAKE_SOURCE_DIR}/src/button_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/byte_buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/crc32.cpp"
    "${CMAKE_SOURCE_DIR}/src/device_id.cpp"
    "${CMAKE_SOURCE_DIR}/src/dockable_pane_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/font_size.cpp"
    "${CMAKE_SOURCE_DIR}/src/genesis_editor.cpp"
    "${CMAKE_SOURCE_DIR}/src/grid_layout_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/gui.cpp"
    "${CMAKE_SOURCE_DIR}/src/gui_window.cpp"
    "${CMAKE_SOURCE_DIR}/src/id_map.cpp"
    "${CMAKE_SOURCE_DIR}/src/key_event.cpp"
    "${CMAKE_SOURCE_DIR}/src/label.cpp"
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/menu_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/mixer_node.cpp"
    "${CMAKE_SOURCE_DIR}/src/mixer_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/ordered_map_file.cpp"
    "${CMAKE_SOURCE_DIR}/src/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/png_image.cpp"
    "${CMAKE_SOURCE_DIR}/src/project.cpp"
    "${CMAKE_SOURCE_DIR}/src/project_props_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/random.cpp"
    "${CMAKE_SOURCE_DIR}/src/render_job.cpp"
    "${CMAKE_SOURCE_DIR}/src/render_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/resource_bundle.cpp"
    "${CMAKE_SOURCE_DIR}/src/resources_tree_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/scroll_bar_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/select_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/sequencer_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/settings_file.cpp"
    "${CMAKE_SOURCE_DIR}/src/sha_256_hasher.cpp"
    "${CMAKE_SOURCE_DIR}/src/shader_program.cpp"
    "${CMAKE_SOURCE_DIR}/src/shader_program_manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/sort_key.cpp"
    "${CMAKE_SOURCE_DIR}/src/spritesheet.cpp"
    "${CMAKE_SOURCE_DIR}/src/static_geometry.cpp"
    "${CMAKE_SOURCE_DIR}/src/string.cpp"
    "${CMAKE_SOURCE_DIR}/src/sunken_box.cpp"
    "${CMAKE_SOURCE_DIR}/src/tab_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/texture.cpp"
    "${CMAKE_SOURCE_DIR}/src/text_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/track_editor_widget.cpp"
    "${CMAKE_SOURCE_DIR}/src/util.cpp"
    "${CMAKE_SOURCE_DIR}/src/warning.cpp"
    "${CMAKE_SOURCE_DIR}/src/widget.cpp"
)

set(UNICODE_HPP "${CMAKE_BINARY_DIR}/unicode.hpp")

set(GENERATE_UNICODE_DATA_SOURCES
    "${CMAKE_SOURCE_DIR}/src/byte_buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/error.cpp"
    "${CMAKE_SOURCE_DIR}/src/generate_unicode_data.cpp"
    "${CMAKE_SOURCE_DIR}/src/util.cpp"
)

set(TEST_SOURCES
    "${CMAKE_SOURCE_DIR}/src/audio_file.cpp"
    "${CMAKE_SOURCE_DIR}/src/audio_graph.cpp"
    "${CMAKE_SOURCE_DIR}/src/byte_buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/crc32.cpp"
    "${CMAKE_SOURCE_DIR}/src/delay.cpp"
    "${CMAKE_SOURCE_DIR}/src/device_id.cpp"
    "${CMAKE_SOURCE_DIR}/src/error.cpp"
    "${CMAKE_SOURCE_DIR}/src/genesis.cpp"
    "${CMAKE_SOURCE_DIR}/src/id_map.cpp"
    "${CMAKE_SOURCE_DIR}/src/midi_hardware.cpp"
    "${CMAKE_SOURCE_DIR}/src/mixer_node.cpp"
    "${CMAKE_SOURCE_DIR}/src/ordered_map_file.cpp"
    "${CMAKE_SOURCE_DIR}/src/os.cpp"
    "${CMAKE_SOURCE_DIR}/src/project.cpp"
    "${CMAKE_SOURCE_DIR}/src/random.cpp"
    "${CMAKE_SOURCE_DIR}/src/resample.cpp"
    "${CMAKE_SOURCE_DIR}/src/ring_buffer.cpp"
    "${CMAKE_SOURCE_DIR}/src/settings_file.cpp"
    "${CMAKE_SOURCE_DIR}/src/sha_256_hasher.cpp"
    "${CMAKE_SOURCE_DIR}/src/sort_key.cpp"
    "${CMAKE_SOURCE_DIR}/src/string.cpp"
    "${CMAKE_SOURCE_DIR}/src/synth.cpp"
    "${CMAKE_SOURCE_DIR}/src/util.cpp"
    "${CMAKE_SOURCE_DIR}/src/warning.cpp"
    "${CMAKE_SOURCE_DIR}/test/ordered_map_file_test.cpp"
    "${CMAKE_SOURCE_DIR}/test/ring_buffer_test.cpp"
    "${CMAKE_SOURCE_DIR}/test/thread_safe_queue_test.cpp"
    "${CMAKE_SOURCE_DIR}/test/unit_tests.cpp"
)

find_package(Threads)
if(Threads_FOUND)
    set(STATUS_THREADS "OK")
else(Threads_FOUND)
    set(STATUS_THREADS "not found")
endif(Threads_FOUND)


find_package(ffmpeg)
if(AVCODEC_FOUND)
    include_directories(${AVCODEC_INCLUDE_DIRS})
    set(STATUS_LIBAVCODEC "OK")
else()
    set(STATUS_LIBAVCODEC "not found")
endif()
if(AVFORMAT_FOUND)
    include_directories(${AVFORMAT_INCLUDE_DIRS})
    set(STATUS_LIBAVFORMAT "OK")
else()
    set(STATUS_LIBAVFORMAT "not found")
endif()
if(AVUTIL_FOUND)
    include_directories(${AVUTIL_INCLUDE_DIRS})
    set(STATUS_LIBAVUTIL "OK")
else()
    set(STATUS_LIBAVUTIL "not found")
endif()

find_package(FreeType)
if(FREETYPE_FOUND)
    include_directories(${FREETYPE_INCLUDE_DIR})
    set(STATUS_FREETYPE "OK")
else()
    set(STATUS_FREETYPE "not found")
endif()

find_package(PNG)
if(PNG_FOUND)
    include_directories(${PNG_INCLUDE_DIR})
    set(STATUS_PNG "OK")
else()
    set(STATUS_PNG "not found")
endif()

find_package(rucksack)
if(RUCKSACK_FOUND)
    include_directories(${RUCKSACK_INCLUDE_DIR})
    set(STATUS_RUCKSACK "OK")
else()
    set(STATUS_RUCKSACK "not found")
endif()

find_package(LaxJson)
if(LAXJSON_FOUND)
    include_directories(${LAXJSON_INCLUDE_DIR})
    set(STATUS_LAXJSON "OK")
else()
    set(STATUS_LAXJSON "not found")
endif()

find_package(SoundIo)
if(SOUNDIO_FOUND)
    include_directories(${SOUNDIO_INCLUDE_DIR})
    set(STATUS_SOUNDIO "OK")
else()
    set(STATUS_SOUNDIO "not found")
endif()

find_package(GLFW)
if(GLFW_FOUND)
    include_directories(${GLFW_INCLUDE_DIR})
    set(STATUS_GLFW "OK")
else()
    set(STATUS_GLFW "not found")
endif()

find_package(epoxy)
if(EPOXY_FOUND)
    include_directories(${EPOXY_INCLUDE_DIR})
    set(STATUS_EPOXY "OK")
else()
    set(STATUS_EPOXY "not found")
endif()

find_package(rhash)
if(RHASH_FOUND)
    include_directories(${RHASH_INCLUDE_DIR})
    set(STATUS_RHASH "OK")
else()
    set(STATUS_RHASH "not found")
endif()


if(APPLE)
    set(GENESIS_HAVE_ALSA false)
    set(STATUS_ALSA "OK")
else()
    set(GENESIS_HAVE_ALSA true)
    find_package(ALSA)
    if(ALSA_FOUND)
        include_directories(${ALSA_INCLUDE_DIRS})
        set(STATUS_ALSA "OK")
    else()
        set(STATUS_ALSA "not found")
    endif()
endif()

# GTFO, -lstdc++ !!
set(CMAKE_CXX_IMPLICIT_LINK_LIBRARIES "")
set(CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES "")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -Werror -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror -pedantic")


set(LIB_CFLAGS "-std=c++11 -fno-exceptions -fno-rtti -fvisibility=hidden -Wall -Werror=strict-prototypes -Werror=old-style-definition -Werror=missing-prototypes -Wno-c99-extensions")
set(EXAMPLE_CFLAGS "-std=c99 -Wall")
set(TEST_CFLAGS "${LIB_CFLAGS} -fprofile-arcs -ftest-coverage")
set(TEST_LDFLAGS "-fprofile-arcs -ftest-coverage")

configure_file (
    "${CMAKE_SOURCE_DIR}/src/config.h.in"
    ${CONFIGURE_OUT_FILE}
)

add_executable(generate_unicode_data ${GENERATE_UNICODE_DATA_SOURCES})
set_target_properties(generate_unicode_data PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${LIB_CFLAGS}
)

set(UNICODE_DATA_URL "ftp://ftp.unicode.org/Public/UNIDATA/UnicodeData.txt")
set(UNICODE_DATA_TXT "/usr/share/unicode/UnicodeData.txt")
if(NOT EXISTS ${UNICODE_DATA_TXT})
    set(UNICODE_DATA_TXT "${CMAKE_BINARY_DIR}/UnicodeData.txt")
    file(DOWNLOAD ${UNICODE_DATA_URL} ${UNICODE_DATA_TXT}
        EXPECTED_HASH SHA1=5d4b8269e0b86b01c049613aca50cfa5ee410957)
endif()
add_custom_command(
    OUTPUT ${UNICODE_HPP}
    COMMAND generate_unicode_data ARGS ${UNICODE_DATA_TXT} ${UNICODE_HPP}
    DEPENDS ${UNICODE_DATA_TXT}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_library(libgenesis_shared SHARED ${LIBGENESIS_SOURCES} ${UNICODE_HPP})
set_target_properties(libgenesis_shared PROPERTIES
    OUTPUT_NAME genesis
    SOVERSION ${LIBGENESIS_VERSION_MAJOR}
    VERSION ${LIBGENESIS_VERSION}
    COMPILE_FLAGS ${LIB_CFLAGS}
    LINKER_LANGUAGE C
)
include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_SOURCE_DIR}/test"
)

target_link_libraries(libgenesis_shared
    ${FFMPEG_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${RHASH_LIBRARY}
    ${SOUNDIO_LIBRARY}
    m
)
target_link_libraries(libgenesis_shared ${CMAKE_THREAD_LIBS_INIT})
install(TARGETS libgenesis_shared DESTINATION lib)


add_library(libgenesis_static STATIC ${LIBGENESIS_SOURCES} ${UNICODE_HPP})
set_target_properties(libgenesis_static PROPERTIES
    OUTPUT_NAME genesis
    COMPILE_FLAGS ${LIB_CFLAGS}
    LINKER_LANGUAGE C
)
install(TARGETS libgenesis_static DESTINATION lib)

install(FILES
    ${LIBGENESIS_HEADERS}
    ${CONFIGURE_OUT_FILE}
    DESTINATION "include/genesis")

# Example Programs

add_executable(list_devices example/list_devices.c)
set_target_properties(list_devices PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${EXAMPLE_CFLAGS})
target_link_libraries(list_devices libgenesis_shared)

add_executable(normalize_audio example/normalize_audio.c)
set_target_properties(normalize_audio PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${EXAMPLE_CFLAGS})
target_link_libraries(normalize_audio libgenesis_shared m)

add_executable(list_supported_formats example/list_supported_formats.c)
set_target_properties(list_supported_formats PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${EXAMPLE_CFLAGS})
target_link_libraries(list_supported_formats libgenesis_shared)

add_executable(synth example/synth.c)
set_target_properties(synth PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${EXAMPLE_CFLAGS})
target_link_libraries(synth libgenesis_shared)

add_executable(delay example/delay.c)
set_target_properties(delay PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${EXAMPLE_CFLAGS})
target_link_libraries(delay libgenesis_shared)

add_executable(play example/play.c)
set_target_properties(play PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${EXAMPLE_CFLAGS})
target_link_libraries(play libgenesis_shared)


set(ASSETS_JSON_FILE "${CMAKE_SOURCE_DIR}/src/assets.json")
set(RESOURCES_FILE "${CMAKE_BINARY_DIR}/resources.bundle")
add_custom_target(rucksack_bundle ALL
    ${RUCKSACK_BINARY} bundle --prefix ${CMAKE_SOURCE_DIR} ${ASSETS_JSON_FILE} ${RESOURCES_FILE}
    DEPENDS ${ASSETS_JSON_FILE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_executable(genesis ${GENESIS_SOURCES} ${UNICODE_HPP})
set_target_properties(genesis PROPERTIES
    LINKER_LANGUAGE CXX
    COMPILE_FLAGS ${LIB_CFLAGS}
)
target_link_libraries(genesis libgenesis_shared
    ${FREETYPE_LIBRARY}
    ${PNG_LIBRARY}
    ${RUCKSACK_LIBRARY}
    ${GLFW_LIBRARY}
    ${EPOXY_LIBRARY}
    ${LAXJSON_LIBRARY}
    ${RHASH_LIBRARY}
    ${FFMPEG_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${RHASH_LIBRARY}
    ${SOUNDIO_LIBRARY}
    -lstdc++
)
install(TARGETS genesis DESTINATION bin)


enable_testing()
add_executable(unit_tests ${TEST_SOURCES} ${UNICODE_HPP})
target_link_libraries(unit_tests
    ${CMAKE_THREAD_LIBS_INIT}
    ${LAXJSON_LIBRARY}
    ${FFMPEG_LIBRARIES}
    ${ALSA_LIBRARIES}
    ${RHASH_LIBRARY}
    ${SOUNDIO_LIBRARY}
    m
    -lstdc++
)
set_target_properties(unit_tests PROPERTIES
    LINKER_LANGUAGE C
    COMPILE_FLAGS ${TEST_CFLAGS}
    LINK_FLAGS ${TEST_LDFLAGS}
)
add_test(UnitTests unit_tests)


add_custom_target(coverage
    DEPENDS unit_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND lcov --directory . --zerocounters --rc lcov_branch_coverage=1
    COMMAND ./unit_tests
    COMMAND lcov --directory . --capture --output-file coverage.info --rc lcov_branch_coverage=1
    COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info.cleaned --rc lcov_branch_coverage=1
    COMMAND genhtml -o coverage coverage.info.cleaned --rc lcov_branch_coverage=1
    COMMAND rm coverage.info coverage.info.cleaned
)



message("\n"
    "Installation Summary\n"
    "--------------------\n"
    "* Install Directory            : ${CMAKE_INSTALL_PREFIX}\n"
    "* Build Type                   : ${CMAKE_BUILD_TYPE}\n"
)

message(
    "System Dependencies\n"
    "-------------------\n"
    "* threads                      : ${STATUS_THREADS}\n"
    "* libavformat                  : ${STATUS_LIBAVFORMAT}\n"
    "* libavcodec                   : ${STATUS_LIBAVCODEC}\n"
    "* libavutil                    : ${STATUS_LIBAVUTIL}\n"
    "* freetype                     : ${STATUS_FREETYPE}\n"
    "* libpng                       : ${STATUS_PNG}\n"
    "* rucksack                     : ${STATUS_RUCKSACK}\n"
    "* liblaxjson                   : ${STATUS_LAXJSON}\n"
    "* libsoundio                   : ${STATUS_SOUNDIO}\n"
    "* GLFW                         : ${STATUS_GLFW}\n"
    "* libepoxy                     : ${STATUS_EPOXY}\n"
    "* ALSA                         : ${STATUS_ALSA}\n"
    "* librhash                     : ${STATUS_RHASH}\n"
)
