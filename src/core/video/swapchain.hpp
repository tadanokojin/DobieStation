#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP
#include "vulkan/vulkan.h"
#include "vulkancontext.hpp"

namespace Vulkan
{
	struct FramebufferObject
	{
		VkImage image;
		VkImageView view = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
		VkCommandBuffer command_buffer = VK_NULL_HANDLE;
	};
	struct SwapChainInfo
    {
		VkSurfaceCapabilitiesKHR capabilities;
		SurfaceFormatList formats;
		PresentModeList present_modes;
	};

	using FramebufferList = std::vector<FramebufferObject>;
    
	class SwapChain
    {
        private:
            Context* context;
			SwapChainInfo info;
            VkPresentModeKHR present_mode;
            VkSurfaceFormatKHR surface_format;
            VkExtent2D extent;
			VkCommandPool command_pool;

			VkSwapchainKHR swapchain = VK_NULL_HANDLE;
			VkSemaphore present_sema = VK_NULL_HANDLE;
			VkSemaphore render_sema = VK_NULL_HANDLE;

			FramebufferList framebuffer_objects;
    
            bool vsync;

			uint32_t image_index = 0;

			ImageList get_chain_images();

            void get_present_mode_support();
            void get_format_support();
            void get_surface_capabilities();
            void select_present_mode();
            void select_format();
            void select_extent();

			void create_semaphores();
			void create_swapchain();
			void setup_images();
            void create_image_views();
			void create_command_buffers();

			void destroy_semaphores();
			void destroy_swapchain();
			void destroy_image_views();

			void acquire_next_image();
			void present();
        public:
            SwapChain(Context* ctx);
            ~SwapChain();

            void reset();
            void destroy();
			void draw_frame();

			VkImage get_current_image() const { return framebuffer_objects[image_index].image; };
    };
}
#endif