#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "bios.hpp"
#include <emulator.hpp>

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture SDL_Texture;
typedef union SDL_Event SDL_Event;

class Application
{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;

    bool running;
    int tex_w, tex_h;

    BiosReader bios;
    Emulator emu;

    bool init();
    void free();
    bool frame();
    void handle_event(SDL_Event& event);

public:
    Application();
    ~Application() = default;

    int run();
};

#endif//__APPLICATION_HPP__
