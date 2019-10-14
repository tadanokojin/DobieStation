#ifndef __APPLICATION_HPP__
#define __APPLICATION_HPP__

#include "params.hpp"
#include "bios.hpp"
#include "window.hpp"
#include <emulator.hpp>
#include <chrono>

typedef union SDL_Event SDL_Event;
typedef struct _SDL_GameController SDL_GameController;
using std::chrono::high_resolution_clock;

class Application
{
private:
    Window     window;
    BiosReader bios;
    Emulator   emu;

    bool running;
    long fps_counter;
    static constexpr auto sec = std::chrono::seconds(1);
    high_resolution_clock::time_point next_tick;

    SDL_GameController* pad;
    int32_t joy_id;

    bool init(Params& params);
    void free();
    bool frame();
    void handle_event(SDL_Event& event);

    void key_event(int32_t keycode, bool down);
    void button_event(int button, bool down);
    void joystick_event(int axis, int16_t value);

    enum class RomType { NONE, ELF, ISO, CSO };
    bool open_rom(const char* path);

public:
    Application();
    ~Application() = default;

    int run(Params& params);
};

#endif//__APPLICATION_HPP__
