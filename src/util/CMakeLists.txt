set(TARGET DobieUtils)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    scoped_lib.cpp
)

set(HEADERS
	scoped_lib.hpp
	wsi.hpp
)

add_library(${TARGET} ${SOURCES} ${HEADERS})
add_library(Dobie::Utils ALIAS ${TARGET})
target_include_directories(${TARGET} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)