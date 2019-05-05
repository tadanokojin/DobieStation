#ifndef VULKANUTILS_HPP
#define VULKANUTILS_HPP

#include "../emulator.hpp"
#include "vulkan/vulkan.h"
namespace Vulkan
{
    void log_result(const char* function_name, VkResult res, const char* msg);
    const char* result_to_string(VkResult res);
    #define VK_PANIC(res, msg) Vulkan::log_result(__func__, res, msg)
}
#endif