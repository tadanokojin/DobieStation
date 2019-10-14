set(TARGET DobieSDL)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)


set(SOURCES
	bios.cpp
	window.cpp
	application.cpp
	params.cpp
    main.cpp)

set(HEADERS
	bios.hpp
	window.hpp
	params.hpp
	application.hpp)

add_executable(${TARGET} ${SOURCES} ${HEADERS})
target_compile_options(${TARGET} PRIVATE ${DOBIE_FLAGS})
target_include_directories(${TARGET} PRIVATE
	${CMAKE_SOURCE_DIR}/src/core
	${SDL2_INCLUDE_DIRS})
target_link_libraries(${TARGET} Dobie::Core ${SDL2_LIBRARIES})
