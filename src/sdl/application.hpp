#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "params.hpp"
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

    bool init(Params& params);
    void free();
    bool frame();
    void handle_event(SDL_Event& event);

    enum class RomType { NONE, ELF, ISO, CSO };
    bool open_rom(const char* path);

public:
    Application();
    ~Application() = default;

    int run(Params& params);
};

#endif//__APPLICATION_HPP__
