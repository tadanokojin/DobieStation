#ifndef VULKANLOADER_HPP
#define VULKANLOADER_HPP

// macro hell
#define VK_NO_PROTOTYPES

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN // don't include useless nonsense
#define NOMINMAX // don't include stupid macros
#define VK_USE_PLATFORM_WIN32_KHR // this includes windows.h for us
#endif

#ifdef __APPLE__
#define VK_USE_PLATFORM_MACOS_MVK
#endif

#ifdef HAVE_X11
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#include "vulkan/vulkan.h"

// Generates externs
// For example
// VK_MODULE_FUNC( vkCreateInstance )
// will expand to
// extern PFN_vkCreateInstance vkCreateInstance;
#define VK_GLOBAL_FUNC(func) extern PFN_##func func;
#define VK_MODULE_FUNC(func) extern PFN_##func func;
#define VK_INSTANCE_FUNC(func) extern PFN_##func func;
#define VK_DEVICE_FUNC(func) extern PFN_##func func;
#include "vulkan.inl"
#undef VK_GLOBAL_FUNC
#undef VK_MODULE_FUNC
#undef VK_INSTANCE_FUNC
#undef VK_DEVICE_FUNC

namespace Vulkan
{
    bool load_vulkan();
    bool load_instance_functions(VkInstance instance);
    bool load_device_functions(VkDevice device);
    void release_vulkan();
}
#endif