#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "bios.hpp"
#include "window.hpp"
#include <emulator.hpp>

class Application
{
private:
    Window window;
    BiosReader bios;
    Emulator emu;

    bool running;

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
