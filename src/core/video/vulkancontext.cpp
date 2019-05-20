#include "vulkanloader.hpp"
#include "vulkancontext.hpp"
#include "../emulator.hpp"

namespace Vulkan
{
    Context::Context(WindowSystem::Info info)
        : wsi(info), instance(VK_NULL_HANDLE),
        messenger(VK_NULL_HANDLE), device(VK_NULL_HANDLE),
        surface(VK_NULL_HANDLE)
    {
    }

    Context::~Context()
    {
    }

    void Context::destroy()
    {
        if(device != VK_NULL_HANDLE)
            vkDestroyDevice(device, nullptr);

        if (surface != VK_NULL_HANDLE)
            vkDestroySurfaceKHR(instance, surface, nullptr);

        if (messenger != VK_NULL_HANDLE)
            vkDestroyDebugUtilsMessengerEXT(instance, messenger, nullptr);

        if(instance != VK_NULL_HANDLE)
            vkDestroyInstance(instance, nullptr);

        release_vulkan();
    }

    void Context::reset()
    {
        destroy();

        create_instance(true, true);
        create_surface();
        select_device();
        create_device();
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL report_callback(
        VkDebugUtilsMessageSeverityFlagBitsEXT severity,
        VkDebugUtilsMessageTypeFlagsEXT type,
        const VkDebugUtilsMessengerCallbackDataEXT* data,
        void* user_data
    )
    {
            fprintf(stderr, "[vulkan validation layer] %s\n", data->pMessage);

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

        instance_create_info.enabledLayerCount = static_cast<uint32_t>(
            instance_info.enabled_layers.size()
        );
        instance_create_info.ppEnabledLayerNames =
            instance_info.enabled_layers.data();

        if (enable_report)
            instance_info.enable_reporting();

        // enable surface support
        instance_info.enable_extension(
            VK_KHR_SURFACE_EXTENSION_NAME, true
        );

        // Enable Windows specific surface support
        #ifdef VK_USE_PLATFORM_WIN32_KHR
        instance_info.enable_extension(
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME, true
        );
        #endif

        // Enable Linux specific surface support
        #ifdef VK_USE_PLATFORM_XLIB_KHR
        instance_info.enable_extension(
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME, true
        );
        #endif

        // Enable MacOS specfic surface support
        #ifdef VK_USE_PLATFORM_MACOS_MVK
        instance_info.enable_extension(
            VK_MVK_MACOS_SURFACE_EXTENSION_NAME, true
        );
        #endif

        instance_create_info.enabledExtensionCount = static_cast<uint32_t>(
            instance_info.enabled_extensions.size()
        );
        instance_create_info.ppEnabledExtensionNames =
            instance_info.enabled_extensions.data();


        // This is the debug report callback
        // it assigns the function we will use to
        // get information from the driver
        VkDebugUtilsMessengerCreateInfoEXT callback_info = {};
        callback_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        callback_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        callback_info.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        callback_info.pfnUserCallback = &report_callback;

        if (enable_report)
        {
            instance_create_info.pNext = static_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&callback_info);
        }

        VkResult res = vkCreateInstance(
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
            res = vkCreateDebugUtilsMessengerEXT(
                instance, &callback_info, nullptr, &messenger
            );

            if (res != VK_SUCCESS)
                VK_PANIC(res, "vkCreateDebugReportCallbackEXT");
        }
    }

    void Context::create_surface()
    {
        #ifdef VK_USE_PLATFORM_WIN32_KHR
        VkWin32SurfaceCreateInfoKHR surface_info = {};
        surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        surface_info.hwnd = reinterpret_cast<HWND>(wsi.render_surface);
        surface_info.hinstance = GetModuleHandle(NULL);

        VkResult res = vkCreateWin32SurfaceKHR(
            instance, &surface_info, nullptr, &surface
        );

        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateWin32SurfaceKHR");
        #endif

        #ifdef VK_USE_PLATFORM_XLIB_KHR
        Errors::die("Create surface: Linux not implemented yet");
        #endif

        #ifdef VK_USE_PLATFORM_MACOS_MVK
        Errors::die("Create surface: MacOS not implemented yet");
        #endif
    }

    void Context::add_available_gpu(VkPhysicalDevice device)
    {
        // Note
        // We likely don't need all this information
        // But I grab it all anyway
        GPUInfo info = {};
        info.surface = surface;
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

        info.populate_queue_indices();

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

            if (!gpu.has_graphics_queue() || !gpu.has_present_queue())
                return false;

            gpu.check_extension_support();

            if (!gpu.has_extension_support(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
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
        queue_create_info.queueFamilyIndex = selected_gpu.graphics_queue_index;
        queue_create_info.queueCount = 1;

        float prio = 1.0f;
        queue_create_info.pQueuePriorities = &prio;

        VkPhysicalDeviceFeatures device_features = {};

        VkDeviceCreateInfo device_create_info = {};
        device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        device_create_info.pQueueCreateInfos = &queue_create_info;
        device_create_info.queueCreateInfoCount = 1;
        device_create_info.pEnabledFeatures = &selected_gpu.features;

        selected_gpu.enable_extension(
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, true
        );

        device_create_info.ppEnabledExtensionNames = selected_gpu.enabled_extensions.data();
        device_create_info.enabledExtensionCount = static_cast<uint32_t>(
            selected_gpu.enabled_extensions.size()
        );

        // Not really needed but we'll specify them anyway for compat
        device_create_info.ppEnabledLayerNames = instance_info.enabled_layers.data();
        device_create_info.enabledLayerCount = static_cast<uint32_t>(
            instance_info.enabled_layers.size()
        );

        VkResult res = vkCreateDevice(
            selected_gpu.device, &device_create_info, nullptr, &device
        );
        if (res != VK_SUCCESS)
            VK_PANIC(res, "vkCreateDevice");

        if (!load_device_functions(device))
            Errors::die("Could not load device functions");
    }
}