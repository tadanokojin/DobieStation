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
VK_INSTANCE_FUNC( vkGetPhysicalDeviceSurfaceSupportKHR )
VK_INSTANCE_FUNC( vkGetPhysicalDeviceSurfacePresentModesKHR )
VK_INSTANCE_FUNC( vkGetPhysicalDeviceSurfaceCapabilitiesKHR )
VK_INSTANCE_FUNC( vkGetPhysicalDeviceSurfaceFormatsKHR )

#ifdef VK_USE_PLATFORM_WIN32_KHR
VK_INSTANCE_FUNC( vkCreateWin32SurfaceKHR )
#endif

VK_INSTANCE_FUNC( vkCreateDebugUtilsMessengerEXT )
VK_INSTANCE_FUNC( vkDestroyDebugUtilsMessengerEXT )
VK_INSTANCE_FUNC( vkDestroySurfaceKHR )
#endif

#ifdef VK_DEVICE_FUNC
VK_DEVICE_FUNC( vkDestroyDevice )
VK_DEVICE_FUNC( vkGetDeviceQueue )
VK_DEVICE_FUNC( vkAllocateMemory )
VK_DEVICE_FUNC( vkBindBufferMemory )
VK_DEVICE_FUNC( vkCreateBuffer )
VK_DEVICE_FUNC( vkDestroyBuffer )
VK_DEVICE_FUNC( vkBindImageMemory )
VK_DEVICE_FUNC( vkCreateSwapchainKHR )
VK_DEVICE_FUNC( vkGetSwapchainImagesKHR )
VK_DEVICE_FUNC( vkDestroySwapchainKHR )
VK_DEVICE_FUNC( vkAcquireNextImageKHR )
VK_DEVICE_FUNC( vkCreateImageView )
VK_DEVICE_FUNC( vkDestroyImageView )
VK_DEVICE_FUNC( vkCreateFramebuffer )
VK_DEVICE_FUNC( vkDestroyFramebuffer )
VK_DEVICE_FUNC( vkCreateCommandPool )
VK_DEVICE_FUNC( vkDestroyCommandPool )
VK_DEVICE_FUNC( vkAllocateCommandBuffers )
VK_DEVICE_FUNC( vkFreeCommandBuffers )
VK_DEVICE_FUNC( vkQueuePresentKHR )
VK_DEVICE_FUNC( vkQueueSubmit )
VK_DEVICE_FUNC( vkCreateSemaphore )
VK_DEVICE_FUNC( vkDestroySemaphore )
VK_DEVICE_FUNC( vkBeginCommandBuffer )
VK_DEVICE_FUNC( vkCmdPipelineBarrier )
VK_DEVICE_FUNC( vkCmdClearColorImage )
VK_DEVICE_FUNC( vkEndCommandBuffer )
#endif