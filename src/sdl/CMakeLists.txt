set(TARGET DobieSDL)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    main.cpp)

set(HEADERS "")

add_executable(${TARGET} ${SOURCES} ${HEADERS})
target_compile_options(${TARGET} PRIVATE ${DOBIE_FLAGS})
target_link_libraries(${TARGET} Dobie::Core)
