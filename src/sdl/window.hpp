#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__

#include <cstdint>

class Application;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_WindowEvent SDL_WindowEvent;
typedef struct SDL_Rect SDL_Rect;

class Window
{
private:
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;

    uint32_t window_id;

    int tex_w, tex_h,
        win_w, win_h;
    double display_ratio, window_ratio;

    void aspect_correct_horz(SDL_Rect& rect) const;
    void aspect_correct_vert(SDL_Rect& rect) const;

public:
    Window();
    inline ~Window() { close(); }

    void handle_event(SDL_WindowEvent& event);
    constexpr SDL_Window* sdl_window() { return window; }

    bool open();
    void close();

    void set_title(const char* title);

    void resize_display(int inner_w, int inner_h, int disp_w, int disp_h);
    void update_texture(void* pixels);
    void present();
};

#endif//__WINDOW_HPP__
