#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP
#include "vulkan/vulkan.h"
#include "vulkancontext.hpp"

namespace Vulkan
{
    class SwapChain
    {
        private:
            Context* context;

            VkSwapchainKHR swapchain;
            VkPresentModeKHR present_mode;
            VkSurfaceFormatKHR surface_format;
            VkExtent2D extent;

            ImageList images;
            ImageViewList image_views;
    
            bool vsync;

            void get_present_mode_support();
            void get_format_support();
            void get_surface_capabilities();
            void select_present_mode();
            void select_format();
            void select_extent();
        public:
            struct SwapChainInfo
            {
                VkSurfaceCapabilitiesKHR capabilities;
                SurfaceFormatList formats;
                PresentModeList present_modes;
            } info;

            SwapChain(Context* ctx);
            ~SwapChain();

            void reset();
            void destroy();

            void create_swapchain();
            void setup_images();
    };
}
#endif