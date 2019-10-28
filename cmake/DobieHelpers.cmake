function(dobie_cxx_compile_options TARGET)

    set(DOBIE_GNU_FLAGS
        -Wall -Wundef -Wsign-compare -Wconversion -Wstrict-aliasing -Wtype-limits

        # These probably should be fixed instead of disabled,
        # but doing so to keep the warning count more managable for now.
        -Wno-reorder -Wno-unused-variable -Wno-unused-value

        # Required on Debug configuration and all configurations on OSX, Dobie WILL crash otherwise.
        $<$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CONFIG:Debug>>:-fomit-frame-pointer>

        $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-but-set-variable> # GNU only warning

        # Might be useful for debugging:
        #-fomit-frame-pointer -fwrapv -fno-delete-null-pointer-checks -fno-strict-aliasing -fvisibility=hidden
    )
    set(DOBIE_MSVC_FLAGS
        /W4 # Warning level 4
    )

    target_compile_options(${TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:${DOBIE_GNU_FLAGS}>
        $<$<CXX_COMPILER_ID:MSVC>:${DOBIE_MSVC_FLAGS}>)

    # Needed to avoid ruining global scope with Windows.h on win32
    target_compile_definitions(${TARGET} PRIVATE
        $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN NOMINMAX>
        $<$<CXX_COMPILER_ID:MSVC>:VC_EXTRALEAN>)

endfunction()

function(dobie_add_glslang SUBDIR)
    if (NOT APPLE)

        # override options
        set(BUILD_TESTING OFF CACHE BOOL "Build the testing tree.")
        set(BUILD_EXTERNAL OFF CACHE BOOL "Build external dependencies in /External")
        set(SKIP_GLSLANG_INSTALL ON CACHE BOOL "Skip installation")
        set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "Builds glslangValidator and spirv-remap")
        set(ENABLE_HLSL OFF CACHE BOOL "Enables HLSL input support")

        # hide irrelevent variables from the GUI
        mark_as_advanced(BUILD_EXTERNAL BUILD_SHARED_LIBS BUILD_TESTING
            ENABLE_EMSCRIPTEN_ENVIRONMENT_NODE ENABLE_EMSCRIPTEN_SINGLE_FILE
            ENABLE_GLSLANG_BINARIES ENABLE_GLSLANG_WEB_DEVEL ENABLE_GLSLANG_DEVEL
            ENABLE_HLSL ENABLE_OPT ENABLE_PCH ENABLE_SPVREMAPPER
            LLVM_USE_CRT_DEBUG LLVM_USE_CRT_MINSIZEREL LLVM_USE_CRT_RELEASE LLVM_USE_CRT_RELWITHDEBINFO
            LLVM_USE_CRT_INSTALL
            SKIP_GLSLANG_INSTALL USE_CCACHE)

        add_subdirectory(${SUBDIR})

        # place under the externals folder
        set_property(TARGET glslang PROPERTY FOLDER External/glslang)
        set_property(TARGET OGLCompiler PROPERTY FOLDER External/glslang)
        set_property(TARGET OSDependent PROPERTY FOLDER External/glslang)
        set_property(TARGET SPIRV PROPERTY FOLDER External/glslang)
        set_property(TARGET SPVRemapper PROPERTY FOLDER External/glslang)

        # add to externals namespace
        add_library(Ext::glslang ALIAS glslang)
        add_library(Ext::SPVRemapper ALIAS SPVRemapper)

    endif()
endfunction()
