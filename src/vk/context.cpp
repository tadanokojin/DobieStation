#include "context.hpp"

namespace Vulkan
{

static VKAPI_ATTR VkBool32 debug_cb
(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*severity*/,
    VkDebugUtilsMessageTypeFlagsEXT /*type*/,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void* /*user_data*/
)
{
    LOG_VIDEO("---Debug Layer---");
    LOG_VIDEO("%s", data->pMessage);
    return VK_FALSE;
}

void Context::reset()
{
    LOG_VIDEO("Context reset");
    Vulkan::load();

    name_list_t enabled_layers;
    name_list_t enabled_extensions;

    // just enabling this for now
    enabled_layers.push_back("VK_LAYER_LUNARG_standard_validation");

    // just enabling these for now
    enabled_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    enabled_extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
    enabled_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#else
#error not implemented
#endif

    VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = &debug_cb;

    // TODO
    // Do we need vk 1.1?
    // probably not for now.
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "DobieStation";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "DobieStation";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.ppEnabledLayerNames = enabled_layers.data();
    instance_info.enabledLayerCount = static_cast<u32>(enabled_layers.size());
    instance_info.ppEnabledExtensionNames = enabled_extensions.data();
    instance_info.enabledExtensionCount = static_cast<u32>(enabled_extensions.size());
    instance_info.pNext = &debug_info;

    CHECK_RESULT(vkCreateInstance(&instance_info, nullptr, &m_instance));

    #define VK_INSTANCE_FUNC(func)                                        \
    if( !(func = (PFN_##func)vkGetInstanceProcAddr(m_instance, #func)) )  \
    {                                                                     \
        LOG_VIDEO("Failed to load instance symbol at %s", #func);         \
    }                                                                     \
                                                                          \
    LOG_VIDEO("Loading instance symbol %s (0x%x)", #func, func);          \
    
    #include "vulkan.inl"

    CHECK_RESULT(vkCreateDebugUtilsMessengerEXT(m_instance, &debug_info, nullptr, &m_messenger));
}

gpu_list_t& Context::enum_gpus()
{
    LOG_VIDEO("Searching for vulkan devices...");

    u32 gpu_count = 0;
    CHECK_RESULT(vkEnumeratePhysicalDevices(
        m_instance, &gpu_count, nullptr
    ));
    
    m_gpus.resize(gpu_count);

    physical_device_list_t physical_devices(gpu_count);
    CHECK_RESULT(vkEnumeratePhysicalDevices(
        m_instance, &gpu_count, physical_devices.data())
    );

    for (u32 i = 0; i < static_cast<u32>(m_gpus.size()); ++i)
    {
        m_gpus[i].reset(m_instance, physical_devices[i]);
    }

    return m_gpus;
}

void Context::destroy()
{
    LOG_VIDEO("Context shutdown");

    if (m_messenger != VK_NULL_HANDLE)
        vkDestroyDebugUtilsMessengerEXT(m_instance, m_messenger, nullptr);

    if (m_instance != VK_NULL_HANDLE)
        vkDestroyInstance(m_instance, nullptr);

    Vulkan::unload();
}

VkInstance Context::instance() const noexcept
{
    return m_instance;
}
}