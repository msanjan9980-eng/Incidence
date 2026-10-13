cmake_minimum_required(VERSION 3.24)

project(incidence
    VERSION 0.1.0
    DESCRIPTION "You have no gun. You have a normal vector."
    LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Global settings
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ---------------------------------------------------------------------------
# Compiler flags — C++ only
#
# Determinism discipline: no fast-math, no FMA contraction, no exceptions,
# no RTTI.
# ---------------------------------------------------------------------------
set(INCIDENCE_CXX_FLAGS "")
if(MSVC)
    list(APPEND INCIDENCE_CXX_FLAGS
        /W4 /permissive- /Zc:__cplusplus /Zc:preprocessor
        /fp:strict /fp:except-
        /GR- /EHs-c- /utf-8)
else()
    list(APPEND INCIDENCE_CXX_FLAGS
        -Wall -Wextra -Wpedantic
        -fno-fast-math -ffp-contract=off
        -fno-exceptions -fno-rtti
        -fvisibility=hidden)
endif()

# ---------------------------------------------------------------------------
# Compiler flags — C only (sokol_impl.c)
#
# sokol's C code is portable but does not compile cleanly under -Wpedantic;
# we suppress that one warning family for the C translation unit only.
# ---------------------------------------------------------------------------
set(INCIDENCE_C_FLAGS "")
if(NOT MSVC)
    list(APPEND INCIDENCE_C_FLAGS
        -Wall -Wextra
        -Wno-pedantic
        -fno-fast-math -ffp-contract=off)
endif()

# ---------------------------------------------------------------------------
# sokol
# ---------------------------------------------------------------------------
include(cmake/sokol.cmake)

# ---------------------------------------------------------------------------
# Main target
# ---------------------------------------------------------------------------
add_executable(incidence
    src/main.cpp
    src/sokol_impl.c
    src/app/app.cpp
    src/platform/platform.cpp)

target_include_directories(incidence PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SOKOL_INCLUDE_DIR})

# Apply C++ flags to C++ sources, C flags to C sources.
set_source_files_properties(
    src/main.cpp
    src/app/app.cpp
    src/platform/platform.cpp
    PROPERTIES COMPILE_OPTIONS "${INCIDENCE_CXX_FLAGS}")

set_source_files_properties(
    src/sokol_impl.c
    PROPERTIES COMPILE_OPTIONS "${INCIDENCE_C_FLAGS}")

# ---------------------------------------------------------------------------
# Platform-specific system libraries
# ---------------------------------------------------------------------------
if(WIN32)
    target_link_libraries(incidence PRIVATE
        user32 gdi32 ole32 d3d11 dxgi)
elseif(APPLE)
    target_link_libraries(incidence PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework Metal"
        "-framework QuartzCore")
elseif(UNIX AND NOT EMSCRIPTEN)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(incidence PRIVATE
        X11::X11 OpenGL::GL dl m pthread)
endif()

# ---------------------------------------------------------------------------
# Copy assets next to the binary on native platforms
# ---------------------------------------------------------------------------
if(NOT EMSCRIPTEN)
    add_custom_command(TARGET incidence POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:incidence>/assets
        COMMENT "Copying assets next to binary")
endif()