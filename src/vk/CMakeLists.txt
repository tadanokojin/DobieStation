set(TARGET DobieVK)

set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    command_buffer.cpp
    context.cpp
    device.cpp
    loader.cpp
    swapchain.cpp
)

set(HEADERS
    command_buffer.hpp
    context.hpp
    device.hpp
    loader.hpp
    swapchain.hpp
    vulkan.inl
)

add_library(${TARGET} ${SOURCES} ${HEADERS})
add_library(Dobie::VK ALIAS ${TARGET})
target_link_libraries(${TARGET} Dobie::Utils)
target_include_directories(${TARGET} PUBLIC ${CMAKE_SOURCE_DIR}/src)
