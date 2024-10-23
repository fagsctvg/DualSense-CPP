cmake_minimum_required(VERSION 3.16)

# Project name and version
project(ds5cpp VERSION 1.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(BUILD_TESTS "Build the test suite" ON)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# FetchContent to include the hidapi dependency
include(FetchContent)

FetchContent_Declare(
    ghidapi
    GIT_REPOSITORY https://github.com/libusb/hidapi.git
    GIT_TAG master
    GIT_PROGRESS ON
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(ghidapi)

# Add include directories for hidapi
include_directories(${ghidapi_SOURCE_DIR} ${ghidapi_BINARY_DIR})

# Source files
set(SOURCES
    src/DS5_Input.cpp
    src/DS5_Output.cpp
    src/DS_CRC32.cpp
    src/Helpers.cpp
    src/IO.cpp
)

# Add the library target
add_library(ds5cpp STATIC ${SOURCES})

# Link the hidapi library
target_link_libraries(ds5cpp hidapi)

# Tests
if (BUILD_TESTS)
    enable_testing()
    add_executable(ds5cpp_test test/wWinMain.cpp)
    
    # Link the test executable to the library and include directories
    target_link_libraries(ds5cpp_test ds5cpp hidapi)
    target_include_directories(ds5cpp_test PRIVATE ${ghidapi_SOURCE_DIR} ${ghidapi_BINARY_DIR})
    
    add_test(NAME ds5cpp_test COMMAND ds5cpp_test)
endif()
