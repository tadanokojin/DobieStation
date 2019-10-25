set(TARGET DobieVK)

set(CMAKE_CXX_STANDARD 11)


set(SOURCES
    command_buffer.cpp
    context.cpp
    device.cpp
    loader.cpp
    swapchain.cpp)

set(HEADERS
    command_buffer.hpp
    context.hpp
    device.hpp
    loader.hpp
    swapchain.hpp
    vulkan.inl)


add_library(${TARGET} ${SOURCES} ${HEADERS})
add_library(Dobie::VK ALIAS ${TARGET})

target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(${TARGET} Dobie::Utils)
target_compile_options(${TARGET} PRIVATE ${DOBIE_FLAGS})

if (${CMAKE_BUILD_TYPE} MATCHES "Debug" OR
    ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")

    target_compile_definitions(${TARGET} PRIVATE VIDEO_DEBUG_LOG)
endif()
