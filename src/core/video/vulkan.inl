#ifdef VK_GLOBAL_FUNC
VK_GLOBAL_FUNC( vkGetInstanceProcAddr )
#endif

#ifdef VK_MODULE_FUNC
VK_MODULE_FUNC( vkCreateInstance )
VK_MODULE_FUNC( vkEnumerateInstanceLayerProperties )
VK_MODULE_FUNC( vkEnumerateInstanceExtensionProperties )
#endif

#ifdef VK_INSTANCE_FUNC
VK_INSTANCE_FUNC( vkDestroyInstance )
VK_INSTANCE_FUNC( vkEnumeratePhysicalDevices )
VK_INSTANCE_FUNC( vkGetPhysicalDeviceProperties )
VK_INSTANCE_FUNC( vkGetPhysicalDeviceFeatures )
VK_INSTANCE_FUNC( vkGetPhysicalDeviceQueueFamilyProperties )
VK_INSTANCE_FUNC( vkCreateDevice )
VK_INSTANCE_FUNC( vkGetDeviceProcAddr )
VK_INSTANCE_FUNC( vkEnumerateDeviceExtensionProperties )
VK_INSTANCE_FUNC( vkGetPhysicalDeviceMemoryProperties )

#ifdef VK_USE_PLATFORM_WIN32_KHR
VK_INSTANCE_FUNC( vkCreateWin32SurfaceKHR )
#endif

VK_INSTANCE_FUNC( vkCreateDebugReportCallbackEXT )
VK_INSTANCE_FUNC( vkDestroyDebugReportCallbackEXT )
VK_INSTANCE_FUNC( vkDebugReportMessageEXT )
#endif

#ifdef VK_DEVICE_FUNC
VK_DEVICE_FUNC( vkDestroyDevice )
VK_DEVICE_FUNC( vkGetDeviceQueue )
VK_DEVICE_FUNC( vkAllocateMemory )
VK_DEVICE_FUNC( vkBindBufferMemory )
VK_DEVICE_FUNC( vkCreateBuffer )
VK_DEVICE_FUNC( vkDestroyBuffer )
#endif