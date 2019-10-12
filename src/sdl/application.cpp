#include "application.hpp"
#include <SDL.h>


Application::Application() :
    running(false)
{

}


bool Application::init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
        return false;

    // load bios
    if (!bios.open("bios.bin"))
        return false;

    // start emulator
    emu.reset();
    emu.load_BIOS(bios.data());

    emu.reset();
    if (!emu.load_CDVD("mbaa.cso", CDVD_CONTAINER::CISO))
        return false;

    emu.set_skip_BIOS_hack(LOAD_DISC);
    emu.set_ee_mode(CPU_MODE::JIT);
    emu.set_vu1_mode(CPU_MODE::JIT);

    // open window
    if (!window.open())
        return false;

    return running = true;
}

void Application::free()
{
    window.close();
    SDL_Quit();
}

bool Application::frame()
{
    // handle events
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0)
        handle_event(event);

    int w = 0, h = 0, iw = 0, ih = 0;
    uint32_t* fb = nullptr;

    // run emulation
    try
    {
        emu.run();
        emu.get_inner_resolution(iw, ih);
        emu.get_resolution(w, h);
        fb = emu.get_framebuffer();
    }
    catch (non_fatal_error& err)
    {
        printf("! non-fatal emulation error occurred !\n! %s\n", err.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Non-fatal emulation error", err.what(), window.sdl_window());
    }
    catch (Emulation_error& err)
    {
        printf("!!! FATAL emulation error occurred, stopping execution !!!\n!!! %s\n", err.what());
        emu.print_state();
        fflush(stdout);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal emulation error", err.what(), window.sdl_window());
        return running = false;
    }

    // present frame
    window.resize_display(iw, ih, w, h);
    window.update_texture(fb);
    window.present();

    return true;
}

void Application::handle_event(SDL_Event& event)
{
    switch(event.type)
    {
    case (SDL_QUIT):
        running = false;
        return;

    default:
        return;
    }
}


int Application::run()
{
    if (!init())
    {
        free();
        return 1;
    }

    int err;
    while (running)
        if (!frame())
            {
                free();
                return -1;
            }

    free();
    return 0;
}
