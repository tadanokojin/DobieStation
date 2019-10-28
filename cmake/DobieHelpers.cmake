function(dobie_win32_lean_and_mean TARGET)
    if (WIN32)
        # Needed to avoid ruining global scope with Windows.h on win32
        target_compile_definitions(${TARGET} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
        if (MSVC)
            target_compile_definitions(${TARGET} PRIVATE VC_EXTRALEAN)
        endif()
    endif()
endfunction()

function(dobie_cxx_compile_options TARGET)
    unset(DOBIE_FLAGS)

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
        ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR
        ${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")

        set(DOBIE_FLAGS
            -Wall -Wundef -Wsign-compare -Wconversion -Wstrict-aliasing -Wtype-limits

            # These probably should be fixed instead of disabled,
            # but doing so to keep the warning count more managable for now.
            -Wno-reorder -Wno-unused-variable -Wno-unused-value

            # Might be useful for debugging:
            #-fomit-frame-pointer -fwrapv -fno-delete-null-pointer-checks -fno-strict-aliasing -fvisibility=hidden
        )

        if (${CMAKE_BUILD_TYPE} MATCHES "Debug" OR
            ${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")

            # Required on Debug configuration and all configurations on OSX, Dobie WILL crash otherwise.
            set(DOBIE_FLAGS ${DOBIE_FLAGS} -fomit-frame-pointer)
        endif()

        if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
            set(DOBIE_FLAGS ${DOBIE_FLAGS} -Wno-unused-but-set-variable) # GNU only warning
        endif()

        set(THREADS_PREFER_PTHREAD_FLAG ON) # -pthreads on GNU-like compilers

    elseif (MSVC)
        set(DOBIE_FLAGS /W4) # Warning level 4

    endif()

    if (DOBIE_FLAGS)
        target_compile_options(${TARGET} PRIVATE ${DOBIE_FLAGS})
    endif()

    dobie_win32_lean_and_mean(${TARGET})
endfunction()
