#ifndef WSI_HPP
#define WSI_HPP
namespace WindowSystem
{
    enum class Type
    {
        Unknown, // Default to tell us system isn't supported
        DWM,     // Windows
        Quartz,  // MacOS
        X11      // Linux and other Unix/Unix-like
    };

    struct Info
    {
        Info() = default;

        // Information about the current window manager
        Type type = Type::Unknown;

        // connection to display server (X11 and wayland)
        void* display_connection = nullptr;

        // Varies by platform
        // Windows this will be HWND
        void* render_surface = nullptr;
    };
}
#endif

