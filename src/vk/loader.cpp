#include <cstdio>
#include <cstdarg>
#include <string>

#include "loader.hpp"
#include "util/scoped_lib.hpp"

namespace Vulkan
{
// PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
// PFN_vkCreateInstance vkCreateInstance = nullptr;
// ...
#define VK_GLOBAL_FUNC(func) PFN_##func func = nullptr;
#include "vulkan.inl"

#define VK_MODULE_FUNC(func) PFN_##func func = nullptr;
#include "vulkan.inl"

#define VK_INSTANCE_FUNC(func) PFN_##func func = nullptr;
#include "vulkan.inl"

#define VK_DEVICE_FUNC(func) PFN_##func func = nullptr;
#include "vulkan.inl"

static Util::Dynalib vulkan_module;

bool load()
{
    LOG_VIDEO("Loading vulkan");

    #ifdef VK_USE_PLATFORM_WIN32_KHR
    vulkan_module.open("vulkan-1.dll");
    #endif

    #ifdef VK_USE_PLATFORM_XLIB_KHR
    vulkan_module.open("libvulkan.so.1");
    #endif

    #ifdef VK_USE_PLATFORM_MACOS_MVK
    vulkan_module.open("libvulkan.dylib");
    #endif

    if (!vulkan_module.is_open())
    {
        LOG_VIDEO("Failed to open vulkan library");
        return false;
    }

    // Global Functions
    // Functions loaded by an OS call to get the symbol addr
    #define VK_GLOBAL_FUNC(func)                              \
    if( !(func = (PFN_##func)vulkan_module.get_sym(#func)) )  \
    {                                                         \
        LOG_VIDEO("Failed to load global symbol %s", #func);  \
        return false;                                         \
    }                                                         \
                                                              \
    LOG_VIDEO("Loading global symbol %s", #func, func);       \

    #include "vulkan.inl"

    // Module Functions
    // Functions loaded by a call to vkGetInstanceProcAddr
    // with instance set to null
    #define VK_MODULE_FUNC(func)                                       \
    if( !(func = (PFN_##func)vkGetInstanceProcAddr(nullptr, #func)) )  \
    {                                                                  \
        LOG_VIDEO("Failed to load module symbol at %s", #func);        \
        return false;                                                  \
    }                                                                  \
                                                                       \
    LOG_VIDEO("Loading module symbol %s", #func, func);                \

    #include "vulkan.inl"

    return true;
}

void unload()
{
    LOG_VIDEO("Shutting down vulkan");
    vulkan_module.close();
}

void log(const char* fmt, ...)
{
    std::string out = "[VIDEO] " + std::string(fmt) + "\n";
    std::va_list args;
    va_start(args, fmt);
    vprintf(out.c_str(), args);
    va_end(args);
}

void hook_or_die(const char* /*file*/, int /*line*/, VkResult res)
{
    std::string message;
    bool fatal = false;

    // TODO
    switch (res)
    {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
        return;
    default:
        message = "RESULT_UNKNOWN";
        fatal = true;
    }

    // TODO
    if (fatal)
        throw std::runtime_error(message);
}
}