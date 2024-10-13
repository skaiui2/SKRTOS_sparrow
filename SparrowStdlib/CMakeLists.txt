cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME xSkRTOS)

# Include toolchain file
include("cmake/gcc-arm-none-eabi.cmake")


include_directories(
        SkRTOS/include/
)

file(GLOB_RECURSE SOURCES
        "SkRTOS/src/*.c"
)




# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

file(GLOB_RECURSE SOURCES "Library/*.*" "Start/*.*" "System/*.*"  "User/*.*")
include_directories(Library/inc Start/inc System/inc User/Inc)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME}
        User/src/main.c
        User/src/main.c
        User/src/stm32f10x_it.c
        Start/src/startup_stm32f103xb.s
        Start/src/system_stm32f10x.c
        Start/src/core_cm3.c
        Library/src/stm32f10x_gpio.c
        Library/src/stm32f10x_rcc.c
        System/src/Delay.c
        System/inc/Delay.h
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs")

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

