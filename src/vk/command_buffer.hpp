#pragma once
#include "loader.hpp"
#include "device.hpp"

namespace Vulkan
{
class CommandBuffer
{
    VkCommandBuffer m_handle = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;

    Vulkan::Device* m_device = nullptr;

    bool m_pending = false;
    bool m_open = false;

public:
    void reset(Vulkan::Device* dev);
    void destroy();

    void submit(VkSemaphore wait_sema, VkSemaphore signal_sema, VkPipelineStageFlags stage_mask);
    void begin();
    void end();

    bool open() const noexcept;
    bool pending() const noexcept;
    VkCommandBuffer handle() const noexcept;
};
}