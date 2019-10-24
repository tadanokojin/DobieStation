#pragma once

#include <limits>
#include <array>
#include "loader.hpp"
#include "context.hpp"
#include "device.hpp"

namespace Vulkan
{
class SwapChain
{
    Vulkan::Device* m_device = nullptr;
    Vulkan::display_handle_t m_display = nullptr;
    Vulkan::image_list_t m_images;

    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkSwapchainKHR m_handle = VK_NULL_HANDLE;
    VkSwapchainKHR m_old_handle = VK_NULL_HANDLE;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;

    u32 m_image_index = std::numeric_limits<u32>::max();
public:
    SwapChain() = default;

    void destroy();
    void reset(Vulkan::Device* dev);
    VkImage next(VkSemaphore signal_sema);
    void present(VkSemaphore wait_sema);

    u32 current() const noexcept;
    Vulkan::Device* device() const noexcept;
};
}
