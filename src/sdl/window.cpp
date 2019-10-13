#include "window.hpp"
#include <SDL.h>


constexpr int    DEFAULT_WIDTH  = 640;
constexpr int    DEFAULT_HEIGHT = 448;
constexpr double DEFAULT_RATIO  = 3.0 / 4.0;

constexpr int TEXTURE_PIXELFORMAT = SDL_PIXELFORMAT_ABGR8888;
constexpr int TEXTURE_ACCESSMODE  = SDL_TEXTUREACCESS_STREAMING;

Window::Window() :
    window(nullptr), renderer(nullptr), texture(nullptr),
    window_id(0),
    tex_w(DEFAULT_WIDTH), tex_h(DEFAULT_HEIGHT),
    win_w(DEFAULT_WIDTH), win_h(DEFAULT_HEIGHT),
    display_ratio(DEFAULT_RATIO),
    window_ratio(DEFAULT_HEIGHT / (double)DEFAULT_WIDTH)
{

}


bool Window::open()
{
    // create window
    constexpr int winpos = SDL_WINDOWPOS_UNDEFINED;
    window = SDL_CreateWindow("DobieSDL",
        winpos, winpos, DEFAULT_WIDTH, DEFAULT_HEIGHT,
        SDL_WINDOW_RESIZABLE);
    if (!window)
        return false;
    window_id = SDL_GetWindowID(window);

    // create renderer
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
        return false;

    // create screen texture
    texture = SDL_CreateTexture(renderer, TEXTURE_PIXELFORMAT, TEXTURE_ACCESSMODE, tex_w, tex_h);
    return texture != nullptr;
}

void Window::close()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}


void Window::handle_event(SDL_WindowEvent& event)
{
    if (event.windowID != window_id)
        return;

    switch (event.event)
    {
    case (SDL_WINDOWEVENT_SIZE_CHANGED):
        win_w = event.data1;
        win_h = event.data2;
        if (win_w && win_h)
            window_ratio = win_h / (double)win_w;
        break;

    default: break;
    }
}

void Window::resize_display(int inner_w, int inner_h, int disp_w, int disp_h)
{
    if (disp_w && disp_h)
        display_ratio = disp_h / (double)disp_w;

    if (!inner_w) return;
    if (!inner_h) return;
    if (inner_w == tex_w && inner_h != tex_h) return;

    SDL_DestroyTexture(texture);
    texture = SDL_CreateTexture(renderer, TEXTURE_PIXELFORMAT, TEXTURE_ACCESSMODE, tex_w, tex_h);
    SDL_assert(texture);
}

void Window::update_texture(void* pixels)
{
    if (pixels)
        SDL_UpdateTexture(texture, nullptr, pixels, tex_w * 4);
}


void Window::aspect_correct_horz(SDL_Rect& rect) const
{
    double len = win_h / display_ratio;
    double pos = (win_w - len) / 2.0;

    rect = {(int)round(pos), 0, (int)round(len), win_h};
}

void Window::aspect_correct_vert(SDL_Rect& rect) const
{
    double len = win_w * display_ratio;
    double pos = (win_h - len) / 2.0;

    rect = {0, (int)round(pos), win_w, (int)round(len)};
}

void Window::present()
{
    SDL_Rect dest;

    if (display_ratio == window_ratio)
        dest = {0, 0, win_w, win_h};
    if (display_ratio > window_ratio)
        aspect_correct_horz(dest);
    else
        aspect_correct_vert(dest);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, &dest);
    SDL_RenderPresent(renderer);
}
