#include "window.hpp"
#include <SDL.h>


constexpr int DEFAULT_WIDTH  = 640;
constexpr int DEFAULT_HEIGHT = 448;
constexpr int TEXTURE_PIXELFORMAT = SDL_PIXELFORMAT_ABGR8888;
constexpr int TEXTURE_ACCESSMODE  = SDL_TEXTUREACCESS_STREAMING;

Window::Window() :
    window(nullptr), renderer(nullptr), texture(nullptr),
    tex_w(DEFAULT_WIDTH), tex_h(DEFAULT_HEIGHT)
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


void Window::resize_display(int inner_w, int inner_h, int disp_w, int disp_h)
{
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

void Window::present()
{
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}
