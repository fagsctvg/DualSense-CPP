cmake_minimum_required(VERSION 3.16)

project(dscpp)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

macro(HandleDependency name submodule_dir git_repo git_tag)
    # Check if the submodule directory exists
    if(EXISTS ${submodule_dir})
        message(STATUS "Using local submodule for ${name}.")
        FetchContent_Declare(
            ${name}
            SOURCE_DIR ${submodule_dir}
        )
    else()
        message(STATUS "Submodule ${name} not found, fetching from external source.")
        FetchContent_Declare(
            ${name}
            GIT_REPOSITORY ${git_repo}
            GIT_TAG ${git_tag}
        )
    endif()

    # Make the dependency available
    FetchContent_MakeAvailable(${name})
endmacro()

HandleDependency(hidapi, ${CMAKE_SOURCE_DIR}/hidapi, https://github.com/libusb/hidapi.git, 750bf201ae906eec1dc52c97087c8d0104c091ae)
