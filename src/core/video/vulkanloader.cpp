#include "vulkanloader.hpp"
#include <stdio.h>

// Unfortunately what follows is a bit of a mess
// so I'll explain it as we go for other people
// who need to work in this file

#ifdef WIN32
#define LoadFuncAddress GetProcAddress
#else
#define LoadFuncAddress dlsym
#endif

// This generates a list of variable defintions
// for example
// VK_GLOBAL_FUNC( vkGetInstanceProcAddr )
// will expand to
// PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
#define VK_GLOBAL_FUNC(func) PFN_##func func;
#define VK_MODULE_FUNC(func) PFN_##func func;
#define VK_INSTANCE_FUNC(func) PFN_##func func;
#define VK_DEVICE_FUNC(func) PFN_##func func;
#include "vulkan.inl"
#undef VK_GLOBAL_FUNC
#undef VK_MODULE_FUNC
#undef VK_INSTANCE_FUNC
#undef VK_DEVICE_FUNC

// Differences in OS for handling
// references to dynamically loaded
// libraries

// TODO: We will need to do this for
// other libraries so it would be nice
// to abstract this code elsewhere
#ifdef WIN32
static HMODULE vulkan_module = nullptr;
#else
static void* vulkan_module = nullptr;
#endif

namespace Vulkan
{
    // Loads Vulkan
    // as well as it's global and module functions
    // Do to the nature of Vulkan we have to
    // define different cats of function pointers
    // depending on how we get them.
    bool load_vulkan()
    {
        // Load the dynamic library here
        // I believe both linux and mac use dlopen
        // Mac will require you load moltenvk
        #ifdef WIN32
        vulkan_module = LoadLibrary(L"vulkan-1.dll");
        #else
        vulkan_module = dlopen("libvulkan.so", RTLD_NOW);
        #endif

        if (!vulkan_module)
        {
            fprintf(stderr, "Couldn't load Vulkan");
            return false;
        }

        // This is the global function loader, IE
        // functions which need to be loaded by the OS
        // We only need this for the next loading function
        // we need vkGetInstanceProcAddr
        #define VK_GLOBAL_FUNC(func) \
        if( !(func = (PFN_##func)LoadFuncAddress(vulkan_module, #func)) ) \
        { \
            fprintf(stderr, "Error loading Vulkan global function ##func"); \
            return false; \
        } \
        fprintf(stderr, "Loaded: %s (global)\n", #func); \

        // The above code defines the code uses to load
        // and OS-level function and we include vulkan.inl
        // to expand the code above with our list of function
        // names
        #include "vulkan.inl"
        #undef VK_GLOBAL_FUNC

        // Module functions are functions which are loaded
        // from the module with a call to vkGetInstanceProcAddr
        // note the useage of nullptr for the instance argument
        // this will be important later
        #define VK_MODULE_FUNC(func) \
        if(  !(func = (PFN_##func)vkGetInstanceProcAddr( nullptr, #func )) ) \
        { \
            fprintf(stderr, "Error loading Vulkan module function %s\n", #func); \
            return false; \
        } \
        fprintf(stderr, "Loaded: %s (module)\n", #func); \

        #include "vulkan.inl"
        #undef VK_MODULE_FUNC

        return true;
    }

    // Instance functions are functions loaded
    // by a call to vkGetInstanceProcAddr but
    // require an instance
    bool load_instance_functions(VkInstance instance)
    {
        #define VK_INSTANCE_FUNC(func) \
        if(  !(func = (PFN_##func)vkGetInstanceProcAddr( instance, #func )) ) \
        { \
            fprintf(stderr, "Error loading Vulkan module function %s\n", #func); \
            return false; \
        } \
        fprintf(stderr, "Loaded: %s (instance)\n", #func); \

        #include "vulkan.inl"
        #undef VK_INSTANCE_FUNC

        return true;
    }


    // Device level functions are those which require
    // a call to vkGetDeviceProcAddr and a device
    bool load_device_functions(VkDevice device)
    {
        #define VK_DEVICE_FUNC(func) \
        if(  !(func = (PFN_##func)vkGetDeviceProcAddr( device, #func )) ) \
        { \
            fprintf(stderr, "Error loading Vulkan device function %s\n", #func); \
            return false; \
        } \
        fprintf(stderr, "Loaded %s (device)\n", #func); \

        #include "vulkan.inl"
        #undef VK_DEVICE_FUNC

        return true;
    }

    // Free the library here
    void release_vulkan()
    {
        if (vulkan_module)
        {
            #ifdef WIN32
            FreeLibrary(vulkan_module);
            #else
            dlclose(vulkan_module);
            #endif
        }
    }
}