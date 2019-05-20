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
        for (auto& image_view : image_views)
        {
            if(image_view != VK_NULL_HANDLE)
                vkDestroyImageView(context->get_device(), image_view, nullptr);
        }

        if(swapchain != VK_NULL_HANDLE)
            vkDestroySwapchainKHR(context->get_device(), swapchain, nullptr);
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
        create_swapchain();
        setup_images();
    }

    void SwapChain::get_surface_capabilities()
    {
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
            context->get_physical_device(), context->get_surface(),
            &info.capabilities
        );
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
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            context->get_physical_device(), context->get_surface(),
            &mode_count, nullptr
        );

        if (mode_count == 0)
            return;

        info.present_modes.resize(mode_count);

        vkGetPhysicalDeviceSurfacePresentModesKHR(
            context->get_physical_device(), context->get_surface(),
            &mode_count, info.present_modes.data()
        );
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

    void SwapChain::create_swapchain()
    {
        VkSwapchainCreateInfoKHR swapchain_info = {};
        swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchain_info.surface = context->get_surface();

        // Shoot for double buffered
        uint32_t image_count = 2;
        uint32_t min_image_count = info.capabilities.minImageCount;
        uint32_t max_image_count = info.capabilities.maxImageCount;

        image_count = std::max(min_image_count, std::min(max_image_count, image_count));

        swapchain_info.minImageCount = image_count;
        swapchain_info.imageFormat = surface_format.format;
        swapchain_info.imageColorSpace = surface_format.colorSpace;
        swapchain_info.imageExtent = extent;
        swapchain_info.imageArrayLayers = 1;
        swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
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
            swapchain_info.queueFamilyIndexCount = static_cast<uint32_t>(indices.size());
        }
        
        // Let the device use it's natural orientation
        swapchain_info.preTransform = info.capabilities.currentTransform;
        swapchain_info.presentMode = present_mode;
        swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchain_info.clipped = VK_TRUE;
        swapchain_info.oldSwapchain = VK_NULL_HANDLE; // Todo

        VkResult res = vkCreateSwapchainKHR(
            context->get_device(), &swapchain_info, nullptr, &swapchain
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateSwapchainKHR");
    }

    void SwapChain::setup_images()
    {
        uint32_t count = 0;
        vkGetSwapchainImagesKHR(
            context->get_device(), swapchain,
            &count, nullptr
        );

        images.resize(count);

        vkGetSwapchainImagesKHR(
            context->get_device(), swapchain,
            &count, images.data()
        );

        image_views.resize(count);

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
                nullptr, &image_views[i]
            );

            if (res != VK_SUCCESS)
                VK_PANIC(res, "vkCreateImageView");
        }
    }
}