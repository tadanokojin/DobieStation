#ifndef VK_DEVICE_HPP
#define VK_DEVICE_HPP
#include <string>
#include <vector>
#include <limits>

#include "loader.hpp"
#include "util/wsi.hpp"

namespace Vulkan
{
#ifdef _WIN32
using display_handle_t = HWND;
#else
using display_handle_t = void*;
#endif
using image_list_t = std::vector<VkImage>;

class Gpu
{
    VkInstance m_instance = VK_NULL_HANDLE;
    VkPhysicalDevice m_handle = VK_NULL_HANDLE;
    VkPhysicalDeviceFeatures m_features;
    VkPhysicalDeviceProperties m_info;
    VkPhysicalDeviceMemoryProperties m_mem_info;
public:
    Gpu() = default;
    ~Gpu() = default;

    void reset(VkInstance instance, VkPhysicalDevice physical_device);

    bool descrete() const noexcept;
    std::string name() const noexcept;
    VkPhysicalDeviceLimits get_limits() const noexcept;
    VkInstance instance() const noexcept;
    VkPhysicalDevice handle() const noexcept;
};

using gpu_list_t = std::vector<Vulkan::Gpu>;

class Device
{
    VkDevice m_handle = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkCommandPool m_command_pool = VK_NULL_HANDLE;

    Vulkan::Gpu* m_gpu;
    queue_properties_list_t m_queue_info;
public:
    Device() = default;
    ~Device() = default;

    void reset(Vulkan::Gpu* gpu, Util::WSI wsi);
    void destroy();

    bool valid() const noexcept;

    std::string name() const noexcept;
    VkInstance instance() const noexcept;
    VkDevice handle() const noexcept;
    VkPhysicalDevice physical_device() const noexcept;
    VkQueue queue() const noexcept;
    VkSurfaceKHR surface() const noexcept;
    VkCommandPool command_pool() const noexcept;
};
}
#endif