#include "device.hpp"

namespace Vulkan
{
void Gpu::reset(VkInstance instance, VkPhysicalDevice physical_device)
{
    m_instance = instance;
    m_handle = physical_device;

    vkGetPhysicalDeviceProperties(physical_device, &m_info);
    vkGetPhysicalDeviceFeatures(physical_device, &m_features);

    LOG_VIDEO("Vulkan device found");
    LOG_VIDEO("\tName: %s", m_info.deviceName);
    LOG_VIDEO("\tVendor ID: 0x%x", m_info.vendorID);
    LOG_VIDEO("\tType: 0x%x", m_info.deviceType);
    LOG_VIDEO("\tDriver Version: 0x%x", m_info.driverVersion);
    LOG_VIDEO("\tAPI Version: 0x%x", m_info.apiVersion);
}

bool Gpu::descrete() const noexcept
{
    return m_info.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
}

std::string Gpu::name() const noexcept
{
    return m_info.deviceName;
}

VkPhysicalDeviceLimits Gpu::get_limits() const noexcept
{
    return m_info.limits;
}

VkInstance Gpu::instance() const noexcept
{
    return m_instance;
}

VkPhysicalDevice Gpu::handle() const noexcept
{
    return m_handle;
}

void Device::reset(Vulkan::Gpu* gpu, Util::WSI wsi)
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = nullptr;
    surface_info.hwnd = reinterpret_cast<display_handle_t>(wsi.surface);

    LOG_VIDEO("Creating Win32 surface");
    LOG_VIDEO("\t display: 0x%x", wsi.surface);
    CHECK_RESULT(vkCreateWin32SurfaceKHR(gpu->instance(), &surface_info, nullptr, &m_surface));
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VkXcbSurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    surface_info.connection = reinterpret_cast<xcb_connection_t*>(wsi.connection);
    surface_info.window = *reinterpret_cast<xcb_window_t*>(wsi.surface);

    LOG_VIDEO("Creating Xcb surface");
    LOG_VIDEO("\t display: 0x%x", wsi.surface);
    CHECK_RESULT(vkCreateXcbSurfaceKHR(gpu->instance(), &surface_info, nullptr, &m_surface));
#else
#error not implemented
#endif

    LOG_VIDEO("Creating device: %s", gpu->name().c_str());
    m_gpu = gpu;

    u32 queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu->handle(), &queue_count, nullptr);
    m_queue_info.resize(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu->handle(), &queue_count, m_queue_info.data());

    LOG_VIDEO("Searching Queue Families");
    u32 queue_family_index = std::numeric_limits<u32>::max();
    for (u32 i = 0; i < static_cast<u32>(m_queue_info.size()); ++i)
    {
        const auto& queue = m_queue_info[i];
        const bool has_graphics = queue.queueFlags & VK_QUEUE_GRAPHICS_BIT;

        VkBool32 can_present = VK_FALSE;
        CHECK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(gpu->handle(), i, m_surface, &can_present));

        if (has_graphics && can_present == VK_TRUE)
        {
            LOG_VIDEO("Found graphics queue family");
            LOG_VIDEO("\tFamily: 0x%x", i);
            LOG_VIDEO("\tSize: 0x%x", queue.queueCount);
            LOG_VIDEO("\tFlags: 0x%x", queue.queueFlags);

            queue_family_index = i;
        }
    }

    LOG_VIDEO("Creating queues...");
    float prio = 1.0f;

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pQueuePriorities = &prio;
    queue_info.queueCount = 1;
    queue_info.queueFamilyIndex = queue_family_index;

    name_list_t enabled_extensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.ppEnabledExtensionNames = enabled_extensions.data();
    device_info.enabledExtensionCount = static_cast<u32>(enabled_extensions.size());
    device_info.queueCreateInfoCount = 1;
    device_info.pQueueCreateInfos = &queue_info;

    // deprecated don't use
    device_info.enabledLayerCount = 0;
    device_info.ppEnabledLayerNames = nullptr;

    CHECK_RESULT(vkCreateDevice(gpu->handle(), &device_info, nullptr, &m_handle));

    #define VK_DEVICE_FUNC(func)                                          \
    if( !(func = (PFN_##func)vkGetDeviceProcAddr(m_handle, #func)) )      \
    {                                                                     \
        LOG_VIDEO("Failed to load device symbol at %s", #func);           \
    }                                                                     \
                                                                          \
    LOG_VIDEO("Loading device symbol %s (0x%x)", #func, func);            \

    #include "vulkan.inl"

    // doesn't return result
    vkGetDeviceQueue(m_handle, queue_family_index, 0, &m_queue);

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType =
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_info.queueFamilyIndex = queue_family_index;

    LOG_VIDEO("Creating command pool");
    CHECK_RESULT(vkCreateCommandPool(
        m_handle, &pool_info, nullptr, &m_command_pool)
    );
}

void Device::destroy()
{
    LOG_VIDEO("Device shutdown");

    if (m_command_pool)
        vkDestroyCommandPool(m_handle, m_command_pool, nullptr);

    if (m_handle)
        vkDestroyDevice(m_handle, nullptr);

    if (m_surface)
        vkDestroySurfaceKHR(m_gpu->instance(), m_surface, nullptr);
}

bool Device::valid() const noexcept
{
    return m_handle != VK_NULL_HANDLE;
}

std::string Device::name() const noexcept
{
    return m_gpu->name();
}

VkInstance Device::instance() const noexcept
{
    return m_gpu->instance();
}

VkDevice Device::handle() const noexcept
{
    return m_handle;
}

VkPhysicalDevice Device::physical_device() const noexcept
{
    return m_gpu->handle();
}

VkQueue Device::queue() const noexcept
{
    return m_queue;
}

VkSurfaceKHR Device::surface() const noexcept
{
    return m_surface;
}

VkCommandPool Device::command_pool() const noexcept
{
    return m_command_pool;
}
}