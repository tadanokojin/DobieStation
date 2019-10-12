#ifndef __WINDOW_HPP__
#define __WINDOW_HPP__

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef union SDL_Event SDL_Event;

class Window
{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    int tex_w, tex_h;

public:
    Window();
    inline ~Window() { close(); }

    constexpr SDL_Window* sdl_window() { return window; }

    bool open();
    void close();

    void resize_display(int inner_w, int inner_h, int disp_w, int disp_h);
    void update_texture(void* pixels);
    void present();
};

#endif//__WINDOW_HPP__
