#include "swapchain.hpp"

namespace Vulkan
{
void SwapChain::reset(Vulkan::Device* dev)
{
    m_device = dev;

    VkSurfaceCapabilitiesKHR surface_info = {};
    CHECK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        dev->physical_device(), dev->surface(), &surface_info
    ));

    LOG_VIDEO("Surface Properties:");
    LOG_VIDEO("\tWidth: %i", surface_info.currentExtent.width);
    LOG_VIDEO("\tHeight: %i", surface_info.currentExtent.height);
    LOG_VIDEO("\tMax width: %i", surface_info.maxImageExtent.width);
    LOG_VIDEO("\tMax height: %i", surface_info.maxImageExtent.height);
    LOG_VIDEO("\tMin width: %i", surface_info.minImageExtent.width);
    LOG_VIDEO("\tMin height: %i", surface_info.minImageExtent.height);
    LOG_VIDEO("\tMax Images: %i", surface_info.maxImageCount);
    LOG_VIDEO("\tMin Images: %i", surface_info.minImageCount);

    format_list_t supported_formats;
    u32 count = 0;
    CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_device->physical_device(), m_device->surface(),
        &count, nullptr
    ));

    supported_formats.resize(count);

    CHECK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(
        m_device->physical_device(), m_device->surface(),
        &count, supported_formats.data()
    ));

    LOG_VIDEO("Supported Formats:");
    for (const auto& format : supported_formats)
        LOG_VIDEO("\tFormat: 0x%x", format.format);

    LOG_VIDEO("Initializing swapchain");

    VkSwapchainCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = dev->surface();
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.imageExtent = surface_info.currentExtent;
    info.preTransform = surface_info.currentTransform;
    info.clipped = VK_TRUE;
    info.minImageCount = 3;
    info.imageArrayLayers = 1;
    info.imageUsage =
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.oldSwapchain = m_old_handle;

    CHECK_RESULT(vkCreateSwapchainKHR(
        m_device->handle(), &info, nullptr, &m_handle
    ));

    u32 image_count = 0;
    vkGetSwapchainImagesKHR(
        m_device->handle(), m_handle, &image_count, nullptr
    );

    m_images.resize(image_count);

    vkGetSwapchainImagesKHR(
        m_device->handle(), m_handle, &image_count, m_images.data()
    );

    LOG_VIDEO("Image count: %i", image_count);
}

VkImage SwapChain::next(VkSemaphore signal_sema)
{
    LOG_VIDEO("Next image");
    CHECK_RESULT(vkAcquireNextImageKHR(
        m_device->handle(), m_handle,
        std::numeric_limits<u32>::max(),
        signal_sema, nullptr, &m_image_index
    ));

    return m_images[m_image_index];
}

void SwapChain::present(VkSemaphore wait_sema)
{
    LOG_VIDEO("Present");
    VkPresentInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    info.swapchainCount = 1;
    info.pSwapchains = &m_handle;
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &wait_sema;
    info.pImageIndices = &m_image_index;

    CHECK_RESULT(vkQueuePresentKHR(m_device->queue(), &info));
}

void SwapChain::destroy()
{
    LOG_VIDEO("SwapChain shutdown");

    if(m_handle)
        vkDestroySwapchainKHR(m_device->handle(), m_handle, nullptr);
}

u32 SwapChain::current() const noexcept
{
    return m_image_index;
}

Vulkan::Device* SwapChain::device() const noexcept
{
    return m_device;
}
}