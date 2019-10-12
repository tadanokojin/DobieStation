#include "application.hpp"
#include <SDL.h>


constexpr int DEFAULT_WIDTH  = 640;
constexpr int DEFAULT_HEIGHT = 448;
constexpr int TEXTURE_PIXELFORMAT = SDL_PIXELFORMAT_ABGR8888;
constexpr int TEXTURE_ACCESSMODE  = SDL_TEXTUREACCESS_STREAMING;


Application::Application() :
    window(nullptr), renderer(nullptr), texture(nullptr),
    running(false),
    tex_w(DEFAULT_WIDTH), tex_h(DEFAULT_HEIGHT)
{

}


bool Application::init()
{
    // load bios
    if (!bios.open("bios.bin"))
        return false;

    // start emulator
    emu.reset();
    emu.load_BIOS(bios.data());
    emu.reset();
    emu.load_CDVD("mbaa.cso", CDVD_CONTAINER::CISO);
    emu.set_skip_BIOS_hack(LOAD_DISC);
    emu.set_ee_mode(CPU_MODE::JIT);
    emu.set_vu1_mode(CPU_MODE::JIT);

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
    if (!texture)
        return false;

    return running = true;
}

void Application::free()
{
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

bool Application::frame()
{
    // handle events
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0)
        handle_event(event);

    int w, h, iw, ih;
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
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Non-fatal emulation error", err.what(), window);
    }
    catch (Emulation_error& err)
    {
        printf("!!! FATAL emulation error occurred, stopping execution !!!\n!!! %s\n", err.what());
        emu.print_state();
        fflush(stdout);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal emulation error", err.what(), window);
        return running = false;
    }

    // resize texture if needed
    if ((iw && ih) && (iw != tex_w || ih != tex_h))
    {
        SDL_DestroyTexture(texture);
        texture = SDL_CreateTexture(renderer, TEXTURE_PIXELFORMAT, TEXTURE_ACCESSMODE, tex_w, tex_h);
        SDL_assert(texture);
    }

    // update screen texture
    if (fb)
        SDL_UpdateTexture(texture, nullptr, fb, tex_w * 4);

    // present frame
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

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
