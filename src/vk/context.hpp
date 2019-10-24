#ifndef VK_CONTEXT_HPP
#define VK_CONTEXT_HPP

#include "loader.hpp"
#include "device.hpp"

namespace Vulkan
{

// callback function
// Let's the api tell us about any problems
// like things not being freed properly
static VKAPI_ATTR VkBool32 debug_cb
(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*user_data*/
);

class Context
{
    VkInstance m_instance = VK_NULL_HANDLE;
    gpu_list_t m_gpus;

    VkDebugUtilsMessengerEXT m_messenger = VK_NULL_HANDLE;
public:
    Context() = default;
    ~Context() = default;

    void reset();
    void destroy();

    gpu_list_t& enum_gpus();
    VkInstance instance() const noexcept;
};
}
#endif