#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#include <vector>
#include "vulkanutils.hpp"
#include "../../common/wsi.hpp"

namespace Vulkan
{
    class Context
    {
        private:
            WindowSystem::Info wsi;

            VkInstance instance;
            VkDevice device;
            VkSurfaceKHR surface;
            VkDebugUtilsMessengerEXT messenger;

            InstanceInfo instance_info;
            GPUInfo selected_gpu;

            std::vector<GPUInfo> available_gpus;

			enum QUEUE_INDEX
			{
				GRAPHICS_QUEUE_INDEX,
				PRESENT_QUEUE_INDEX
			};
			std::array<VkQueue, 2> queues; // 0 graphics 1 present

            void add_available_gpu(VkPhysicalDevice device);

            bool find_physical_devices();
        public:
            Context(WindowSystem::Info info);
            ~Context();

            void reset();
            void destroy();

            void create_instance(bool enable_debug_layer, bool enable_report);
            void create_surface();
            void select_device();
            void create_device();

            VkInstance get_instance() const { return instance; };
            VkSurfaceKHR get_surface() const { return surface; }
            VkPhysicalDevice get_physical_device() const { return selected_gpu.device; };
            VkDevice get_device() const { return device; };

            uint32_t get_graphics_queue_family_index() const { return selected_gpu.get_graphics_queue_index(); };
            uint32_t get_present_queue_family_index() const { return selected_gpu.get_present_queue_index(); };
			VkQueue get_present_queue() const { return queues[PRESENT_QUEUE_INDEX]; };
			VkQueue get_graphics_queue() const { return queues[GRAPHICS_QUEUE_INDEX]; };
    };
}
#endif