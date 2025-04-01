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

# Enable testing
enable_testing()

# Find Core Audio and AudioToolbox frameworks on macOS
if(APPLE)
    find_library(CORE_AUDIO_FRAMEWORK CoreAudio)
    find_library(AUDIO_TOOLBOX_FRAMEWORK AudioToolbox)
    set(EXTRA_LIBS ${CORE_AUDIO_FRAMEWORK} ${AUDIO_TOOLBOX_FRAMEWORK})
endif()

# Server source files (excluding main.cpp)
set(SERVER_LIB_SOURCES
    server/src/music_server.cpp
    server/src/client_handler.cpp
    server/src/music_library.cpp
    server/src/wav_file.cpp
)

# Create server library for testing
add_library(music_server_lib STATIC ${SERVER_LIB_SOURCES})
target_include_directories(music_server_lib PUBLIC server/src)
target_link_libraries(music_server_lib ${EXTRA_LIBS})

# Server executable
add_executable(music_server server/src/main.cpp)
target_include_directories(music_server PRIVATE server/src)
target_link_libraries(music_server music_server_lib)

# Client source files (excluding main.cpp)
set(CLIENT_LIB_SOURCES
    client/src/music_client.cpp
    client/src/audio_player.cpp
)

# Create client library for testing
add_library(music_client_lib STATIC ${CLIENT_LIB_SOURCES})
target_include_directories(music_client_lib PUBLIC client/src)
target_link_libraries(music_client_lib ${EXTRA_LIBS})

# Client executable
add_executable(music_client client/src/main.cpp)
target_include_directories(music_client PRIVATE client/src)
target_link_libraries(music_client music_client_lib)

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

# Google Test setup
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Unit tests
file(GLOB TEST_SOURCES "tests/unit/*.cpp")
add_executable(run_unit_tests ${TEST_SOURCES})
target_include_directories(run_unit_tests PRIVATE client/src server/src)
target_link_libraries(run_unit_tests
  GTest::gtest_main
  music_server_lib
  music_client_lib
)

# Integration tests
file(GLOB INTEGRATION_TEST_SOURCES "tests/integration/*.cpp")
if(INTEGRATION_TEST_SOURCES)
  add_executable(run_integration_tests ${INTEGRATION_TEST_SOURCES})
  target_include_directories(run_integration_tests PRIVATE client/src server/src)
  target_link_libraries(run_integration_tests
    GTest::gtest_main
    music_server_lib
    music_client_lib
  )
endif()

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(run_unit_tests)
if(INTEGRATION_TEST_SOURCES)
  gtest_discover_tests(run_integration_tests)
endif()

# Copy test data to build directory
add_custom_command(
    TARGET run_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/tests/test_data
    ${CMAKE_BINARY_DIR}/bin/test_data
)