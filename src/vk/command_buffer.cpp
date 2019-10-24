#include "command_buffer.hpp"

namespace Vulkan
{
void CommandBuffer::reset(Vulkan::Device* dev)
{
    m_device = dev;

    VkCommandBufferAllocateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = m_device->command_pool();
    info.commandBufferCount = 1;
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    CHECK_RESULT(vkAllocateCommandBuffers(m_device->handle(), &info, &m_handle));

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    CHECK_RESULT(vkCreateFence(m_device->handle(), &fence_info, nullptr, &m_fence));
}

void CommandBuffer::begin()
{
    LOG_VIDEO("Opening command buffer 0x%x", m_handle);
    if (m_pending)
    {
        LOG_VIDEO("Command buffer opened while pending. Waiting on fence...");
        vkWaitForFences(
            m_device->handle(), 1, &m_fence,
            true, std::numeric_limits<u32>::max()
        );

        CHECK_RESULT(vkResetFences(m_device->handle(), 1, &m_fence));
        CHECK_RESULT(vkResetCommandBuffer(m_handle, 0));

        m_pending = false;
        LOG_VIDEO("Command buffer ready");
    }

    VkCommandBufferBeginInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    CHECK_RESULT(vkBeginCommandBuffer(m_handle, &info));
    m_open = true;
}

void CommandBuffer::end()
{
    LOG_VIDEO("Closing command buffer 0x%x", m_handle);
    if (!m_open)
    {
        LOG_VIDEO("ERROR attempted to end a command buffer that wasn't recording");
        return;
    }

    CHECK_RESULT(vkEndCommandBuffer(m_handle));
    m_open = false;
}

void CommandBuffer::submit(VkSemaphore wait_sema, VkSemaphore signal_sema, VkPipelineStageFlags stage_mask)
{
    LOG_VIDEO("Submitting command buffer 0x%x", m_handle);
    if (m_open)
    {
        LOG_VIDEO("ERROR attempted to submit an open command buffer");
        return;
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pWaitSemaphores = &wait_sema;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &signal_sema;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pCommandBuffers = &m_handle;
    submit_info.commandBufferCount = 1;
    submit_info.pWaitDstStageMask = &stage_mask;

    CHECK_RESULT(vkQueueSubmit(m_device->queue(), 1, &submit_info, m_fence));
    m_pending = true;
}

void CommandBuffer::destroy()
{
    if (m_fence)
        vkDestroyFence(m_device->handle(), m_fence, nullptr);
}

bool CommandBuffer::open() const noexcept
{
    return m_open;
}

bool CommandBuffer::pending() const noexcept
{
    return m_pending;
}

VkCommandBuffer CommandBuffer::handle() const noexcept
{
    return m_handle;
}
}