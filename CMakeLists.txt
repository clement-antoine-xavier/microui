cmake_minimum_required(VERSION 3.10)

project(
    microui
    VERSION 1.0.0
    DESCRIPTION "MicroUI"
    HOMEPAGE_URL "https://clement-antoine-xavier.github.io/microui/"
    LANGUAGES C
)

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set global C standard and properties
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build" FORCE)
    set_property(
        CACHE CMAKE_BUILD_TYPE
        PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo"
    )
endif()

# Set the files directories
set(HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/headers")
set(SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources")

# Add the source files
file(GLOB_RECURSE SOURCES "${SOURCES_DIR}/*.c")

# Create the main library
add_library(${PROJECT_NAME} ${SOURCES})

# Include the public headers
target_include_directories(
    ${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${HEADERS_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Set the library properties
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${HEADERS_DIR}/microui.h"
)

# Optional components
option(BUILD_EXAMPLES "Build the examples" ON)

if(BUILD_EXAMPLES)
    message(STATUS "Building examples...")
    add_subdirectory(examples)
else()
    message(STATUS "Skipping examples...")
endif()
