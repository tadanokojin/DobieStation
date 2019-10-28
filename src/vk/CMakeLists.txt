set(TARGET DobieVK)

set(CMAKE_CXX_STANDARD 14)
include(DobieHelpers)


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
target_link_libraries(${TARGET} Dobie::Utils Ext::glslang Ext::SPVRemapper)
dobie_cxx_compile_options(${TARGET})

# Quick hack to use bundled Vulkan headers until a proper way to find them is ascertained
if (NOT VULKAN_FOUND)
    target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/ext/vulkan/include)
endif()

# Define 'VIDEO_DEBUG_LOG' in debug configurations
target_compile_definitions(${TARGET}
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:VIDEO_DEBUG_LOG>)
