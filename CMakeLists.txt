cmake_minimum_required(VERSION 3.1)
project(DobieStation
    VERSION 0.1.0)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}") # fix Release build
set(THREADS_PREFER_PTHREAD_FLAG ON) # -pthreads on GNU-like compilers


# Externals
add_subdirectory(ext/libdeflate)
add_subdirectory(ext/imgui)
if (NOT APPLE)
    # glslang options
    set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
    set(BUILD_EXTERNAL OFF CACHE BOOL "Build external dependencies in /External")
    set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "Skip installation")
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Builds glslangValidator and spirv-remap")
    set(ENABLE_HLSL OFF CACHE BOOL "Enables HLSL input support")

    mark_as_advanced(BUILD_EXTERNAL BUILD_SHARED_LIBS BUILD_TESTING
        ENABLE_EMSCRIPTEN_ENVIRONMENT_NODE ENABLE_EMSCRIPTEN_SINGLE_FILE
        ENABLE_GLSLANG_BINARIES ENABLE_GLSLANG_WEB_DEVEL ENABLE_GLSLANG_DEVEL
        ENABLE_HLSL ENABLE_OPT ENABLE_PCH ENABLE_SPVREMAPPER
        LLVM_USE_CRT_DEBUG LLVM_USE_CRT_MINSIZEREL LLVM_USE_CRT_RELEASE LLVM_USE_CRT_RELWITHDEBINFO
        LLVM_USE_CRT_INSTALL
        SKIP_GLSLANG_INSTALL USE_CCACHE)

    add_subdirectory(ext/glslang)
    set_property(TARGET glslang PROPERTY FOLDER External/glslang)
    set_property(TARGET OGLCompiler PROPERTY FOLDER External/glslang)
    set_property(TARGET OSDependent PROPERTY FOLDER External/glslang)
    set_property(TARGET SPIRV PROPERTY FOLDER External/glslang)
    set_property(TARGET SPVRemapper PROPERTY FOLDER External/glslang)
    add_library(Ext::glslang ALIAS glslang)
    add_library(Ext::SPVRemapper ALIAS SPVRemapper)
endif()

# Shared packages
find_package(Threads REQUIRED)

# Modules
add_subdirectory(src/core)
add_subdirectory(src/qt)
add_subdirectory(src/util)
if (NOT APPLE)
    add_subdirectory(src/vk)
endif()


if (MSVC)
    # Use DobieQt as Startup Project instead of ALL_BUILD
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT DobieQt)
endif()

install(TARGETS DobieQt DESTINATION bin)

if (UNIX AND NOT APPLE)
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/data/DobieStation.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/DobieStation.desktop)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/DobieStation.desktop DESTINATION share/applications/)
endif()
