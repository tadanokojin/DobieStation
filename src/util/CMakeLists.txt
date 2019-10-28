set(TARGET Utils)

set(CMAKE_CXX_STANDARD 11)
include(DobieHelpers)


set(SOURCES
    scoped_lib.cpp)

set(HEADERS
    scoped_lib.hpp
    wsi.hpp)


add_library(${TARGET} ${SOURCES} ${HEADERS})
add_library(Dobie::Utils ALIAS ${TARGET})

target_include_directories(${TARGET} PUBLIC ${CMAKE_SOURCE_DIR}/src)
dobie_cxx_compile_options(${TARGET})
dobie_win32_lean_and_mean(${TARGET})
