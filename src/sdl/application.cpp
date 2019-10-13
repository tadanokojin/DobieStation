#include "application.hpp"
#include <SDL.h>


Application::Application() :
    running(false)
{

}


bool Application::open_rom(const char* path)
{
    RomType rom_type = RomType::NONE;
    size_t path_len;
    char ext[4];

    if ((path_len = strlen(path)) < 5)
    {
        fprintf(stderr, "Unable to deduce file extension.\n");
        return false;
    }
    for (size_t i = 0; i < 4; ++i)
        ext[i] = (char)tolower(path[path_len - 4 + i]);
    if (ext[0] != '.')
    {
        fprintf(stderr, "Unable to deduce file extension.\n");
        return false;
    }

    if (strncmp(ext, ".elf", 4) == 0)
        rom_type = RomType::ELF;
    else if (strncmp(ext, ".iso", 4) == 0)
        rom_type = RomType::ISO;
    else if (strncmp(ext, ".cso", 4) == 0)
        rom_type = RomType::CSO;

    switch (rom_type)
    {
    case (RomType::ISO):
        return emu.load_CDVD(path, CDVD_CONTAINER::ISO);
    case (RomType::CSO):
        return emu.load_CDVD(path, CDVD_CONTAINER::CISO);
    default:
    case (RomType::NONE):
        fprintf(stderr, "Unrecognised file extension \"%.4s\".\n", ext);
        return false;
    }
}

bool Application::init(Params& params)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
        return false;

    // load bios
    if (!bios.open(params.bios_path))
    {
        fprintf(stderr, "Fatal: failed to open BIOS image \"%s\".\n", params.bios_path);
        return false;
    }

    // start emulator
    emu.reset();
    emu.load_BIOS(bios.data());

    // load rom
    emu.reset();
    if (!open_rom(params.rom_path))
    {
        fprintf(stderr, "Fatal: failed to open ROM image \"%s\".\n", params.rom_path);
        return false;
    }

    if (!params.bios_boot)
        emu.set_skip_BIOS_hack(LOAD_DISC);

    auto cpu_mode = params.interpreter ? CPU_MODE::INTERPRETER : CPU_MODE::JIT;
    emu.set_ee_mode(cpu_mode);
    emu.set_vu1_mode(cpu_mode);

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
        break;

    case (SDL_WINDOWEVENT):
        window.handle_event(event.window);
        break;

    default: break;
    }
}


int Application::run(Params& params)
{
    if (!init(params))
    {
        free();
        return 1;
    }

    while (running)
        if (!frame())
            {
                free();
                return -1;
            }

    free();
    return 0;
}
