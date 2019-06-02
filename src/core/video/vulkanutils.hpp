#ifndef VULKANUTILS_HPP
#define VULKANUTILS_HPP

#include <vector>
#include <array>
#include <limits>
#include <algorithm>
#include <set>

#include "vulkan/vulkan.h"
namespace Vulkan
{
    void log_result(const char* function_name, VkResult res, const char* msg);
    const char* result_to_string(VkResult res);

    // some utility types
    using NameList = std::vector<const char*>;
    using ExtensionList = std::vector<VkExtensionProperties>;
    using LayerList = std::vector<VkLayerProperties>;
    using PhysicalDeviceList = std::vector<VkPhysicalDevice>;
    using QueuePropertyList = std::vector<VkQueueFamilyProperties>;
    using SurfaceFormatList = std::vector<VkSurfaceFormatKHR>;
    using PresentModeList = std::vector<VkPresentModeKHR>;
    using ImageList = std::vector<VkImage>;
    using ImageViewList = std::vector<VkImageView>;

    // Couple of utility wrapper classes
    class GPUInfo
    {
        public:
            VkPhysicalDevice device;
            VkSurfaceKHR surface;

            ExtensionList available_extensions;
            NameList enabled_extensions;
            QueuePropertyList queue_family_properties;

            VkPhysicalDeviceFeatures features;
            VkPhysicalDeviceProperties properties;

            uint32_t graphics_queue_index;
            uint32_t present_queue_index;

            GPUInfo();
            ~GPUInfo() = default;

            bool has_graphics_queue() const { return graphics_queue_index != std::numeric_limits<uint32_t>::max(); };
            bool has_present_queue() const { return present_queue_index != std::numeric_limits<uint32_t>::max(); };
            bool has_extension_support(const char* name);

            uint32_t get_graphics_queue_index() const { return graphics_queue_index; };
            uint32_t get_present_queue_index() const { return present_queue_index; };

            void check_extension_support();
            void enable_extension(const char* name, bool required);
            void populate_queue_indices();
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

    // utility define for panic
    #define VK_PANIC(res, msg) Vulkan::log_result(__func__, res, msg);
}
#endif