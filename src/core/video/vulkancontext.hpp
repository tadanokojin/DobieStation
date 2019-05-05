#ifndef VULKANCONTEXT_HPP
#define VULKANCONTEXT_HPP

#include <vector>
#include "vulkan/vulkan.h"

namespace Vulkan
{
    using NameList = std::vector<const char*>;
    using ExtensionList = std::vector<VkExtensionProperties>;
    using LayerList = std::vector<VkLayerProperties>;
    using PhysicalDeviceList = std::vector<VkPhysicalDevice>;
    using QueuePropertyList = std::vector<VkQueueFamilyProperties>;

    // Couple of utility wrapper classes
    class GPUInfo
    {
        public:
            VkPhysicalDevice device = VK_NULL_HANDLE;
            ExtensionList available_extensions;

            NameList enabled_extensions;
            QueuePropertyList queue_family_properties;

            VkPhysicalDeviceFeatures features = {};
            VkPhysicalDeviceProperties properties = {};

            GPUInfo() = default;
            ~GPUInfo() = default;

            bool has_graphics_queue();
            uint32_t get_graphics_queue_index();
    };

    class InstanceInfo
    {
        public:
            LayerList available_layers;
            ExtensionList available_extensions;

            NameList enabled_layers;
            NameList enabled_extensions;

            InstanceInfo() = default;
            ~InstanceInfo() = default;

            bool has_layer_support(const char* name);
            bool has_extension_support(const char* name);
            void enable_layer(const char* name, bool required);
            void enable_extension(const char* name, bool required);

            void check_layer_support();
            void check_extension_support();
            void enable_reporting();
            void enable_debug_layer();
    };

    class Context
    {
        private:
            VkInstance instance;
            VkDevice device;
            VkDebugReportCallbackEXT debug_callback;

            InstanceInfo instance_info;
            GPUInfo selected_gpu;

            std::vector<GPUInfo> available_gpus;

            void add_available_gpu(VkPhysicalDevice device);

            bool find_physical_devices();
        public:
            Context();
            ~Context();

            void create_instance(bool enable_debug_layer, bool enable_report);
            void select_device();
            void create_device();

            VkDevice get_device() const { return device; };
            VkInstance get_instance() const { return instance; };
    };
}
#endif