set(TARGET DobieVK)

set(CMAKE_CXX_STANDARD 11)


set(SOURCES
    command_buffer.cpp
    loader.cpp
    context.cpp
    device.cpp
    swapchain.cpp)

set(HEADERS
    command_buffer.hpp
    loader.hpp
    context.hpp
    device.hpp
    swapchain.hpp
    vulkan.inl)


add_library(${TARGET} ${SOURCES} ${HEADERS})
add_library(Dobie::VK ALIAS ${TARGET})

target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_compile_options(${TARGET} PRIVATE ${DOBIE_FLAGS})
