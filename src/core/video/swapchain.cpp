#include "vulkanloader.hpp"
#include "swapchain.hpp"
#include "vulkanutils.hpp" 
#include "../emulator.hpp"

namespace Vulkan
{
    SwapChain::SwapChain(Context* ctx)
        : context(ctx), vsync(true)
    {
    }

    SwapChain::~SwapChain()
    {
    }

    void SwapChain::destroy()
    {
        destroy_image_views();
        destroy_swapchain();
        destroy_semaphores();
    }

    void SwapChain::reset()
    {
        destroy();

        get_surface_capabilities();
        get_format_support();
        get_present_mode_support();
        select_present_mode();
        select_format();
        select_extent();
        create_semaphores();
        create_swapchain();
        setup_images();
        create_image_views();
        create_command_buffers();
    }

    void SwapChain::get_surface_capabilities()
    {
        VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            context->get_physical_device(), context->get_surface(),
            &info.capabilities
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    }

    void SwapChain::get_format_support()
    {
        uint32_t format_count;
        vkGetPhysicalDeviceSurfaceFormatsKHR(
            context->get_physical_device(), context->get_surface(),
            &format_count, nullptr
        );

        if (format_count == 0)
            return;

        info.formats.resize(format_count);

        vkGetPhysicalDeviceSurfaceFormatsKHR(
            context->get_physical_device(), context->get_surface(),
            &format_count, info.formats.data()
        );
    }

    void SwapChain::get_present_mode_support()
    {
        uint32_t mode_count;
        VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(
            context->get_physical_device(), context->get_surface(),
            &mode_count, nullptr
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkGetPhysicalDeviceSurfacePresentModesKHR");

        if (mode_count == 0)
            return;

        info.present_modes.resize(mode_count);

        res = vkGetPhysicalDeviceSurfacePresentModesKHR(
            context->get_physical_device(), context->get_surface(),
            &mode_count, info.present_modes.data()
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkGetPhysicalDeviceSurfacePresentModesKHR");
    }

    void SwapChain::select_format()
    {
        auto has_format = [=](VkSurfaceFormatKHR check_surface_format) {
            for (auto& format : info.formats)
            {
                VkFormat check_format = check_surface_format.format;
                VkColorSpaceKHR check_color_space = check_surface_format.colorSpace;
                
                if (format.format == VK_FORMAT_UNDEFINED)
                    return true;

                if (format.format == check_format && format.colorSpace == check_color_space)
                    return true;
            }

            return false;
        };

        // We'll just try a sane default first
        // I do this check because the device might not care
        // in which case I want to select this format instead
        // of the format in info.formats[0].format which will be VK_FORMAT_UNDEFINED
        // and probably not valid for creating a swapchain
        VkSurfaceFormatKHR format = {};
        format.format = VK_FORMAT_B8G8R8A8_UNORM;
        format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        if (has_format(format))
        {
            surface_format = format;
            return;
        }

        // Probably not a big deal if we don't get the format we want
        // We'll just select the top format
        fprintf(stderr, "SwapChain: Surface format not automatically selected\n");
        surface_format = info.formats[0];
    }

    void SwapChain::select_present_mode()
    {
        auto has_mode = [=](VkPresentModeKHR mode) {
            for (auto& present_mode : info.present_modes)
            {
                if (present_mode == mode)
                    return true;
            }

            return false;
        };

        // First we will check for vsync
        if (vsync && has_mode(VK_PRESENT_MODE_FIFO_KHR))
        {
            present_mode = VK_PRESENT_MODE_FIFO_KHR;
            return;
        }

        // Next let's try for tearing
        if (has_mode(VK_PRESENT_MODE_IMMEDIATE_KHR))
        {
            present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
            return;
        }

        // All else failed let's go for the mode guarenteed
        // by the spec
        fprintf(stderr, "SwapChain: Present mode not automatically selected "
                        "vsync will be enabled\n");
        present_mode = VK_PRESENT_MODE_FIFO_KHR;
    }

    void SwapChain::select_extent()
    {
        // uint32 max means the window manager
        // doesn't care if we specify an extent different
        // from the current window size
        // we still have to obey the limits, though.
        if (info.capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
        {
            extent = info.capabilities.currentExtent;
            return;
        }

        fprintf(stderr, "SwapChain: Extent not automatically selected "
                        "defaulting to 640x448\n");

        const uint32_t width = 640;
        const uint32_t height = 448;

        uint32_t min_width = info.capabilities.minImageExtent.width;
        uint32_t min_height = info.capabilities.minImageExtent.height;

        uint32_t max_width = info.capabilities.maxImageExtent.width;
        uint32_t max_height = info.capabilities.maxImageExtent.height;

        extent.width = std::max(min_width, std::min(max_width, width));
        extent.height = std::max(min_height, std::min(max_height, height));
    }

    void SwapChain::create_semaphores()
    {
        VkSemaphoreCreateInfo sema_info = {};
        sema_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkResult res = vkCreateSemaphore(
            context->get_device(), &sema_info,
            nullptr, &present_sema
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateSemaphore");

        res = vkCreateSemaphore(
            context->get_device(), &sema_info,
            nullptr, &render_sema
        );

        if(res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateSemaphore");
    }

    void SwapChain::create_swapchain()
    {
        VkSwapchainCreateInfoKHR swapchain_info = {};
        swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_info.surface = context->get_surface();

        uint32_t min_image_count = info.capabilities.minImageCount;
        uint32_t max_image_count = info.capabilities.maxImageCount;

        // give us one extra image than min if we can
        uint32_t image_count = min_image_count + 1;
        image_count = std::max(min_image_count, std::min(max_image_count, image_count));

        swapchain_info.minImageCount = image_count;
        swapchain_info.imageFormat = surface_format.format;
        swapchain_info.imageColorSpace = surface_format.colorSpace;
        swapchain_info.imageExtent = extent;
        swapchain_info.imageArrayLayers = 1;
        swapchain_info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;

        // In the event of two seperate indices
        // overwrite part the above
        if (context->get_graphics_queue_family_index() != context->get_present_queue_family_index())
        {
            std::array<uint32_t, 2> indices = {
                context->get_graphics_queue_family_index(),
                context->get_present_queue_family_index()
            };

            swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            // These two are optional if present and graphics are the same
            swapchain_info.pQueueFamilyIndices = indices.data();
            swapchain_info.queueFamilyIndexCount = static_cast<uint32_t>(
                indices.size()
            );
        }
        
        // Let the device use it's natural orientation
        swapchain_info.preTransform = info.capabilities.currentTransform;
        swapchain_info.presentMode = present_mode;
        swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_info.clipped = VK_TRUE;
        swapchain_info.oldSwapchain = VK_NULL_HANDLE; // Todo

        VkResult res = vkCreateSwapchainKHR(
            context->get_device(), &swapchain_info,
            nullptr, &swapchain
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateSwapchainKHR");
    }

    ImageList SwapChain::get_chain_images()
    {
        uint32_t count = 0;
        VkResult res = vkGetSwapchainImagesKHR(
            context->get_device(), swapchain,
            &count, nullptr
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkGetSwapchainImagesKHR");

        ImageList images(count);

        res = vkGetSwapchainImagesKHR(
            context->get_device(), swapchain,
            &count, images.data()
        );

        if(res != VK_SUCCESS)
            VK_PANIC(res, "vkGetSwapchainImagesKHR");

        return images;
    }

    void SwapChain::setup_images()
    {
        ImageList images = get_chain_images();
        framebuffer_objects.resize(images.size());

        for (uint32_t i = 0; i < framebuffer_objects.size(); ++i)
            framebuffer_objects[i].image = images[i];
    }

    void SwapChain::create_image_views()
    {
        ImageList images = get_chain_images();

        for (uint32_t i = 0; i < images.size(); ++i)
        {
            VkImageViewCreateInfo view_info = {};
            view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_info.image = images[i];
            view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
            view_info.format = surface_format.format;
            view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            view_info.subresourceRange.baseMipLevel = 0;
            view_info.subresourceRange.levelCount = 1;
            view_info.subresourceRange.baseArrayLayer = 0;
            view_info.subresourceRange.layerCount = 1;

            VkResult res = vkCreateImageView(
                context->get_device(), &view_info,
                nullptr, &framebuffer_objects[i].view
            );

            if (res != VK_SUCCESS)
                VK_PANIC(res, "vkCreateImageView");
        }
    }

    void SwapChain::create_command_buffers()
    {
        VkCommandPoolCreateInfo command_pool_info = {};
        command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        command_pool_info.queueFamilyIndex =
            context->get_present_queue_family_index();

        VkResult res = vkCreateCommandPool(
            context->get_device(), &command_pool_info,
            nullptr, &command_pool
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateCommandPool");

        std::vector<VkCommandBuffer> cmd_buffers;
        cmd_buffers.resize(framebuffer_objects.size());

        VkCommandBufferAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        alloc_info.commandPool = command_pool;
        alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc_info.commandBufferCount = static_cast<uint32_t>(
            cmd_buffers.size()
        );

        res = vkAllocateCommandBuffers(
            context->get_device(), &alloc_info,
            cmd_buffers.data()
        );
        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkAllocateCommandBuffers");

        for (uint32_t i = 0; i < cmd_buffers.size(); ++i)
            framebuffer_objects[i].command_buffer = cmd_buffers[i];

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        begin_info.pInheritanceInfo = nullptr;

        VkClearColorValue color = {
            {0.2f, 0.6f, 0.86f, 0.0f}
        };

        VkImageSubresourceRange range = {};
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        range.levelCount = 1;
        range.layerCount = 1;

        for(const auto& obj : framebuffer_objects)
        { 
            VkImageMemoryBarrier present_to_clear = {};
            present_to_clear.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            present_to_clear.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            present_to_clear.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            present_to_clear.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            present_to_clear.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            present_to_clear.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            present_to_clear.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            present_to_clear.image = obj.image;
            present_to_clear.subresourceRange = range;

            VkImageMemoryBarrier clear_to_present = {};
            clear_to_present.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            clear_to_present.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            clear_to_present.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            clear_to_present.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            clear_to_present.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            clear_to_present.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            clear_to_present.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            clear_to_present.image = obj.image;
            clear_to_present.subresourceRange = range;

            vkBeginCommandBuffer(obj.command_buffer, &begin_info);
            vkCmdPipelineBarrier(
                obj.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                0, 0, nullptr, 0, nullptr, 1, &present_to_clear
            );

            vkCmdClearColorImage(
                obj.command_buffer, obj.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range
            );

            vkCmdPipelineBarrier(
                obj.command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0, 0, nullptr, 0, nullptr, 1, &clear_to_present
            );

            vkEndCommandBuffer(obj.command_buffer);
        }
    }
    
    void SwapChain::destroy_semaphores()
    {
        if (present_sema != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(
                context->get_device(), present_sema, nullptr
            );
        }

        if (render_sema != VK_NULL_HANDLE)
        {
            vkDestroySemaphore(
                context->get_device(), render_sema, nullptr
            );
        }
    }
    
    void SwapChain::destroy_swapchain()
    {
        if (swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(
                context->get_device(), swapchain, nullptr
            );
        }
    }

    void SwapChain::destroy_image_views()
    {
        for (auto& object : framebuffer_objects)
        {
            if (object.view != VK_NULL_HANDLE)
            {
                vkDestroyImageView(
                    context->get_device(), object.view, nullptr
                );
            }
            // images are cleaned up my the vkDestroySwapchainKHR
        }
    }

    void SwapChain::acquire_next_image()
    {
        VkResult res = vkAcquireNextImageKHR(
            context->get_device(), swapchain,
            std::numeric_limits<uint32_t>::max(),
            present_sema, nullptr, &image_index
        );

        switch (res)
        {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            VK_PANIC(res, "vkAcquireNextImageKHR resize not implemented");
        default:
            VK_PANIC(res, "vkAcquireNextImageKHR");
        }
    }

    void SwapChain::present()
    {
        VkSwapchainKHR swapchains[] = { swapchain };

        VkPresentInfoKHR present_info = {};
        present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present_info.swapchainCount = 1;
        present_info.pSwapchains = swapchains;
        present_info.pImageIndices = &image_index;
        present_info.waitSemaphoreCount = 1;
        present_info.pWaitSemaphores = &render_sema;

        VkResult res = vkQueuePresentKHR(
            context->get_present_queue(),
            &present_info
        );

        switch (res)
        {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            VK_PANIC(res, "vkQueuePresentKHR resize not implemented");
        default:
            VK_PANIC(res, "vkQueuePresentKHR");
        }
    }

    void SwapChain::draw_frame()
    {
        acquire_next_image();

        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.pWaitSemaphores = &present_sema;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &render_sema;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pCommandBuffers = &framebuffer_objects[image_index].command_buffer;
        submit_info.commandBufferCount = 1;

        VkPipelineStageFlags stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        submit_info.pWaitDstStageMask = &stage_mask;

        VkResult res = vkQueueSubmit(
            context->get_present_queue(), 1,
            &submit_info, nullptr
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkQueueSubmit");

        present();
    }
}