find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)

# Set the files directories
set(EXAMPLE_HEADERS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/headers")
set(EXAMPLE_SOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sources")

# Add the source files
file(GLOB_RECURSE EXAMPLE_SOURCES
    "${EXAMPLE_SOURCES_DIR}/*.c"
)

# Create executable
add_executable(simple_application ${EXAMPLE_SOURCES})

# Include the public headers
target_include_directories(simple_application PUBLIC
    $<BUILD_INTERFACE:${EXAMPLE_HEADERS_DIR}>
    $<INSTALL_INTERFACE:include>
)

# Link with microui library
target_link_libraries(simple_application PRIVATE
    microui
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)

# Set common properties
set_target_properties(simple_application PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)
