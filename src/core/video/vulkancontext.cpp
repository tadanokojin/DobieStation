#include "vulkanloader.hpp"
#include "vulkancontext.hpp"
#include "vulkanutils.hpp"
#include "../emulator.hpp"

namespace Vulkan
{
    bool GPUInfo::has_graphics_queue()
    {
        if (get_graphics_queue_index() > queue_family_properties.size())
            return false;

        return true;
    }

    uint32_t GPUInfo::get_graphics_queue_index()
    {
        if (!queue_family_properties.size())
            return UINT32_MAX;

        for (uint32_t i = 0; i < queue_family_properties.size(); ++i)
            if (queue_family_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                return i;

        return UINT32_MAX;
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
        enable_extension(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    void InstanceInfo::enable_debug_layer()
    {
        enable_layer("VK_LAYER_LUNARG_standard_validation");
    }

    Context::Context()
        : instance(VK_NULL_HANDLE),
        debug_callback(VK_NULL_HANDLE),
        device(VK_NULL_HANDLE)
    {
    }

    Context::~Context()
    {
        if(device != VK_NULL_HANDLE)
            vkDestroyDevice(device, nullptr);

        if (debug_callback != VK_NULL_HANDLE)
            vkDestroyDebugReportCallbackEXT(instance, debug_callback, nullptr);

        if(instance != VK_NULL_HANDLE)
            vkDestroyInstance(instance, nullptr);

        release_vulkan();
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL report_callback(
        VkDebugReportFlagsEXT flags,
        VkDebugReportObjectTypeEXT objectType,
        uint64_t object,
        size_t location,
        int32_t messageCode,
        const char* pLayerPrefix,
        const char* pMessage,
        void* pUserData
    )
    {
        if(flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
            fprintf(stderr, "[%s] ERROR %s\n", pLayerPrefix, pMessage);
        if(flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)
            fprintf(stderr, "[%s] WARN %s\n", pLayerPrefix, pMessage);
        if(flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
            fprintf(stderr, "[%s] PERF %s\n", pLayerPrefix, pMessage);

        return VK_FALSE;
    }

    void Context::create_instance(bool enable_debug_layer, bool enable_report)
    {
        if (!load_vulkan())
            Errors::die("Failed to load Vulkan library");

        // This must be called before trying to enable
        // any layers. Otherwise vulkan throws a fit.
        instance_info.check_layer_support();
        instance_info.check_extension_support();

        // Application information
        // mostly stuff for compat
        VkApplicationInfo app_create_info = {};
        app_create_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_create_info.pApplicationName = "DobieStation";
        app_create_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_create_info.pEngineName = "DobieGS Software";
        app_create_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        app_create_info.apiVersion = VK_API_VERSION_1_0;

        // this is where we enable any debugging
        VkInstanceCreateInfo instance_create_info = {};
        instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instance_create_info.pApplicationInfo = &app_create_info;

        if (enable_debug_layer)
            instance_info.enable_debug_layer();

        instance_create_info.enabledLayerCount =
            instance_info.enabled_layers.size();
        instance_create_info.ppEnabledLayerNames =
            instance_info.enabled_layers.data();

        if (enable_report)
            instance_info.enable_reporting();

        instance_info.enable_extension(
            VK_KHR_SURFACE_EXTENSION_NAME, true
        );

        #ifdef VK_USE_PLATFORM_WIN32_KHR
        instance_info.enable_extension(
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true
        );
        #endif

        #ifdef VK_USE_PLATFORM_MACOS_MVK
        instance_info.enable_extension(
            VK_MVK_MACOS_SURFACE_EXTENSION_NAME, true
        );
        #endif

        instance_create_info.enabledExtensionCount =
            instance_info.enabled_extensions.size();
        instance_create_info.ppEnabledExtensionNames =
            instance_info.enabled_extensions.data();

        VkResult res;
        res = vkCreateInstance(
            &instance_create_info, nullptr, &instance
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateInstance");

        // Load all the function pointers for
        // the instance level
        if (!load_instance_functions(instance))
            Errors::die("Failed to load instance functions\n");

        if (enable_report)
        {
            // This is the debug report callback
            // it assigns the function we will use to
            // get information from the driver
            VkDebugReportCallbackCreateInfoEXT callback_info = {};
            callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
            callback_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                VK_DEBUG_REPORT_DEBUG_BIT_EXT;
            callback_info.pfnCallback = &report_callback;

            res = vkCreateDebugReportCallbackEXT(
                instance, &callback_info, nullptr, &debug_callback
            );

            if (res != VK_SUCCESS)
                VK_PANIC(res, "vkCreateDebugReportCallbackEXT");

        }
    }

    void Context::add_available_gpu(VkPhysicalDevice device)
    {
        // Note
        // We likely don't need all this information
        // But I grab it all anyway
        GPUInfo info = {};
        info.device = device;

        vkGetPhysicalDeviceProperties(device, &info.properties);
        vkGetPhysicalDeviceFeatures(device, &info.features);

        uint32_t queue_prop_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_prop_count, nullptr);

        info.queue_family_properties.resize(queue_prop_count);

        vkGetPhysicalDeviceQueueFamilyProperties(
            device, &queue_prop_count,
            info.queue_family_properties.data()
        );

        available_gpus.push_back(info);

        fprintf(stderr, "\t%s\n", info.properties.deviceName);
    }

    bool Context::find_physical_devices()
    {
        uint32_t num_devices = 0;

        VkResult res;
        res = vkEnumeratePhysicalDevices(
            instance, &num_devices, nullptr
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkEnumeratePhysicalDevices");

        if (num_devices == 0)
        {
            fprintf(stderr, "Failed to find any Vulkan ready devices\n");
            return false;
        }

        std::vector<VkPhysicalDevice> physical_devices(num_devices);

        res = vkEnumeratePhysicalDevices(
            instance, &num_devices, physical_devices.data()
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkEnumeratePhysicalDevices");

        fprintf(stderr, "GPUs:\n");
        for (const auto& device : physical_devices)
            add_available_gpu(device);

        return true;
    }

    void Context::select_device()
    {
        // TODO: It would be nice to select a device in the gui
        // but it's too much of a pain right now
        if (!find_physical_devices())
            Errors::die("Failed to find any vulkan devices");

        auto enable_gpu = [=](GPUInfo gpu, VkPhysicalDeviceType type) {
            if (gpu.properties.deviceType != type)
                return false;

            if (!gpu.has_graphics_queue())
                return false;

            selected_gpu = gpu;
            fprintf(stderr, "Selecting: %s\n", gpu.properties.deviceName);

            return true;
        };

        // First try for a discrete gpu
        for (const auto& gpu : available_gpus)
        {
            VkPhysicalDeviceType type =
                VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

            if (enable_gpu(gpu, type))
                return;
        }

        // Try again, this time for an integrated
        // Not ideal, but better than nothing
        for (const auto& gpu : available_gpus)
        {
            VkPhysicalDeviceType type =
                VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;

            if (enable_gpu(gpu, type))
                return;
        }

        // Hail mary
        // I'm not even sure if this check matters
        for (const auto& gpu : available_gpus)
        {
            VkPhysicalDeviceType type =
                VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;

            if (enable_gpu(gpu, type))
                return;
        }

        Errors::die("Could not find suitable GPU");
    }

    void Context::create_device()
    {
        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = selected_gpu.get_graphics_queue_index();
        queue_create_info.queueCount = 1;

        float prio = 1.0f;
        queue_create_info.pQueuePriorities = &prio;

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo device_create_info = {};
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pEnabledFeatures = &selected_gpu.features;
        device_create_info.ppEnabledExtensionNames = selected_gpu.enabled_extensions.data();
        device_create_info.enabledExtensionCount = selected_gpu.enabled_extensions.size();

        // Not really needed but we'll specify them anyway for compat
        device_create_info.ppEnabledLayerNames = instance_info.enabled_layers.data();
        device_create_info.enabledLayerCount = instance_info.enabled_layers.size();

        VkResult res = vkCreateDevice(
            selected_gpu.device, &device_create_info, nullptr, &device
        );
        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateDevice");

        if (!load_device_functions(device))
            Errors::die("Could not load device functions");
    }
}