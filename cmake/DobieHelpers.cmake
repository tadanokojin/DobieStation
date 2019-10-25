function(dobie_win32_lean_and_mean TARGET)
    if (WIN32)
        # Needed to avoid ruining global scope with Windows.h on win32
        target_compile_definitions(${TARGET} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
        if (MSVC)
            target_compile_definitions(${TARGET} PRIVATE VC_EXTRALEAN)
        endif()
    endif()
endfunction()
