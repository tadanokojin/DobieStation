#include "vulkanloader.hpp"
#include "vulkanutils.hpp"
#include "../emulator.hpp"
#include <stdio.h>

namespace Vulkan
{
    void log_result(const char* function_name, VkResult res, const char* msg)
    {
        Errors::die("%s: %s %s", function_name, msg, result_to_string(res));
    }

    const char* result_to_string(VkResult res)
    {
        switch (res)
        {
        case VK_SUCCESS:
            return "VK_SUCCESS";

        case VK_NOT_READY:
            return "VK_NOT_READY";

        case VK_TIMEOUT:
            return "VK_TIMEOUT";

        case VK_EVENT_SET:
            return "VK_EVENT_SET";

        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";

        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";

        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";

        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";

        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";

        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";

        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";

        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";

        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";

        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";

        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";

        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";

        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";

        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";

        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";

        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";

        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";

        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";

        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";

        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";

        default:
            return "UNKNOWN_VK_RESULT";
        }
    }

    GPUInfo::GPUInfo()
        : device(VK_NULL_HANDLE), surface(VK_NULL_HANDLE),
        graphics_queue_index(std::numeric_limits<uint32_t>::max()),
        present_queue_index(std::numeric_limits<uint32_t>::max()),
        features({}), properties({})
    {
    }

    void GPUInfo::check_extension_support()
    {
        uint32_t count = 0;
        vkEnumerateDeviceExtensionProperties(
            device, nullptr,
            &count, nullptr
        );

        available_extensions.resize(count);

        vkEnumerateDeviceExtensionProperties(
            device, nullptr,
            &count, available_extensions.data()
        );

        fprintf(stderr, "Available device extensions:\n");
        for (const auto& extension : available_extensions)
            fprintf(stderr, "\t%s\n", extension.extensionName);
    }

    bool GPUInfo::has_extension_support(const char* name)
    {
        for (const auto& extension : available_extensions)
        {
            if (strcmp(extension.extensionName, name) == 0)
                return true;
        }

        return false;
    }

    void GPUInfo::enable_extension(const char* name, bool required = false)
    {
        if (has_extension_support(name))
        {
            enabled_extensions.push_back(name);

            fprintf(stderr, "Enabling device extension: %s\n", name);
            return;
        }

        if (required)
            Errors::die("Could not enable required device extension %s", name);

        fprintf(stderr, "Tried to enable device extension %s but not supported\n", name);
    }

    void GPUInfo::populate_queue_indices()
    {
        // We need the internal id of each queue
        // this corresponds to the index of the array
        // returned by vkGetPhysicalDeviceQueueFamilyProperties
        uint32_t selected_index = std::numeric_limits<uint32_t>::max();

        for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
        {
            VkBool32 can_present = VK_FALSE;
            bool has_graphics = queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT;

            VkResult res;
             res = vkGetPhysicalDeviceSurfaceSupportKHR(
                 device, i, surface, &can_present
             );

             if (res != VK_SUCCESS)
                 VK_PANIC(res, "vkGetPhysicalDeviceSurfaceSupportKHR");

            // prefer a queue with both
            if (can_present == VK_TRUE && has_graphics)
            {
                graphics_queue_index = i;
                present_queue_index = i;
                break;
            }

            if (has_graphics)
                graphics_queue_index = i;

            if (can_present == VK_TRUE)
                present_queue_index = i;

            // we found both queues no need to keep looking
            if (graphics_queue_index != std::numeric_limits<uint32_t>::max()
                && present_queue_index != std::numeric_limits<uint32_t>::max())
                break;
        }
    }

    void InstanceInfo::check_layer_support()
    {
        uint32_t count = 0;

        VkResult res;
        res = vkEnumerateInstanceLayerProperties(
            &count, nullptr
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkEnumerateInstanceLayerProperties");

        available_layers.resize(count);

        res = vkEnumerateInstanceLayerProperties(
            &count, available_layers.data()
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkEnumerateInstanceLayerProperties");

        fprintf(stderr, "Available instance layers:\n");
        for (const auto& layer : available_layers)
            fprintf(stderr, "\t%s\n", layer.layerName);
    }

    void InstanceInfo::check_extension_support()
    {
        uint32_t count = 0;

        VkResult res;
        res = vkEnumerateInstanceExtensionProperties(
            nullptr, &count, nullptr
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkEnumerateInstanceExtensionProperties");

        available_extensions.resize(count);

        res = vkEnumerateInstanceExtensionProperties(
            nullptr, &count, available_extensions.data()
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkEnumerateInstanceExtensionProperties");

        fprintf(stderr, "Available instance extensions:\n");
        for (const auto& extension : available_extensions)
            fprintf(stderr, "\t%s\n", extension.extensionName);
    }

    bool InstanceInfo::has_layer_support(const char* name)
    {
        for (const auto& layer : available_layers)
        {
            if (strcmp(layer.layerName, name) == 0)
                return true;
        }

        return false;
    }

    bool InstanceInfo::has_extension_support(const char* name)
    {
        for (const auto& extension : available_extensions)
        {
            if (strcmp(extension.extensionName, name) == 0)
                return true;
        }

        return false;
    }

    void InstanceInfo::enable_layer(const char* name, bool required = false)
    {
        if (has_layer_support(name))
        {
            enabled_layers.push_back(name);

            fprintf(stderr, "Enabling layer: %s\n", name);
            return;
        }

        if (required)
            Errors::die("Couldn't enable required layer %s", name);

        fprintf(stderr, "Tried to enable layer %s but not supported\n", name);
    }

    void InstanceInfo::enable_extension(const char* name, bool required = false)
    {
        if (has_extension_support(name))
        {
            enabled_extensions.push_back(name);

            fprintf(stderr, "Enabling extension: %s\n", name);
            return;
        }

        if (required)
            Errors::die("Could not enable required extension %s", name);

        fprintf(stderr, "Tried to enable extension %s but not supported\n", name);
    }

    void InstanceInfo::enable_reporting()
    {
        enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    void InstanceInfo::enable_debug_layer()
    {
        enable_layer("VK_LAYER_LUNARG_standard_validation");
    }
}