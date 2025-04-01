cmake_minimum_required(VERSION 3.12)
project(music_player VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/common/include)

# Find Core Audio and AudioToolbox frameworks on macOS
if(APPLE)
    find_library(CORE_AUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIO_TOOLBOX_FRAMEWORK AudioToolbox)
    set(EXTRA_LIBS ${CORE_AUDIO_FRAMEWORK} ${AUDIO_TOOLBOX_FRAMEWORK})
endif()

# Server
set(SERVER_SOURCES
    server/src/main.cpp
    server/src/music_server.cpp
    server/src/client_handler.cpp
    server/src/music_library.cpp
    server/src/wav_file.cpp
)
add_executable(music_server ${SERVER_SOURCES})
target_include_directories(music_server PRIVATE server/src)
target_link_libraries(music_server ${EXTRA_LIBS})

# Client
set(CLIENT_SOURCES
    client/src/main.cpp
    client/src/music_client.cpp
    client/src/audio_player.cpp
)
add_executable(music_client ${CLIENT_SOURCES})
target_include_directories(music_client PRIVATE client/src)
target_link_libraries(music_client ${EXTRA_LIBS})

# Install targets
install(TARGETS music_server music_client
        RUNTIME DESTINATION bin)

# Create a directory for music files in the build directory
add_custom_command(
    TARGET music_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_BINARY_DIR}/bin/music
)

# Copy any existing music files to the build directory
add_custom_command(
    TARGET music_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/music
    ${CMAKE_BINARY_DIR}/bin/music
)

# Add a custom target for creating a sample WAV file for testing
add_custom_target(create_sample_wav
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/music/Synth\ 108\ Bm\ 2.wav
    ${CMAKE_BINARY_DIR}/bin/music/
    DEPENDS music_server
    COMMENT "Copying sample WAV file to music directory"
)