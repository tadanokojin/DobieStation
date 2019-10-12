#include <SDL.h>
#include <emulator.hpp>
#include "bios.hpp"

#include <cstdio>


constexpr int DEFAULT_WIDTH  = 640;
constexpr int DEFAULT_HEIGHT = 448;
constexpr int TEXTURE_PIXELFORMAT = SDL_PIXELFORMAT_ABGR8888;
constexpr int TEXTURE_ACCESSMODE  = SDL_TEXTUREACCESS_STREAMING;

SDL_Window*   window   = nullptr;
SDL_Renderer* renderer = nullptr;

SDL_Texture* texture = nullptr;
int tex_w = DEFAULT_WIDTH,
	tex_h = DEFAULT_HEIGHT;

BiosReader* bios = nullptr;
Emulator*   emu  = nullptr;
bool running;

int init()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER))
        return 1;

    bios = new BiosReader();
    if (!bios->open("bios.bin"))
    	return 1;

    constexpr int winpos = SDL_WINDOWPOS_UNDEFINED;
    window = SDL_CreateWindow("DobieSDL",
        winpos, winpos, DEFAULT_WIDTH, DEFAULT_HEIGHT,
        SDL_WINDOW_RESIZABLE);
    if (!window)
    {
        SDL_Quit();
        return 1;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    renderer = SDL_CreateRenderer(window, -1, 0);
    if (!renderer)
    {
    	SDL_Quit();
    	return 1;
    }

    texture = SDL_CreateTexture(renderer, TEXTURE_PIXELFORMAT, TEXTURE_ACCESSMODE, tex_w, tex_h);
    if (!texture)
    {
    	SDL_Quit();
    	return 1;
    }

    emu = new Emulator;
    emu->reset();
    emu->load_BIOS(bios->data());
    emu->reset();
    emu->load_CDVD("mbaa.cso", CDVD_CONTAINER::CISO);
    emu->set_skip_BIOS_hack(LOAD_DISC);
    emu->set_ee_mode(CPU_MODE::JIT);
    emu->set_vu1_mode(CPU_MODE::JIT);

    running = true;
    return 0;
}

void handle_event(SDL_Event& event)
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

int loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event) > 0)
        handle_event(event);

    try
    {
        emu->run();

        int w, h, iw, ih;
        emu->get_inner_resolution(iw, ih);
        emu->get_resolution(w, h);

        if ((iw && ih) && (iw != tex_w || ih != tex_h))
        {
            SDL_DestroyTexture(texture);
            texture = SDL_CreateTexture(renderer, TEXTURE_PIXELFORMAT, TEXTURE_ACCESSMODE, tex_w, tex_h);
            SDL_assert(texture);
        }
    }
    catch (non_fatal_error& err)
    {
        printf("! non-fatal emulation error occurred !\n! %s\n", err.what());
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Non-fatal emulation error", err.what(), window);
    }
    catch (Emulation_error& err)
    {
        printf("!!! FATAL emulation error occurred, stopping execution !!!\n!!! %s\n", err.what());
        emu->print_state();
        fflush(stdout);

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Fatal emulation error", err.what(), window);
        running = false;
        return 1;
    }

    SDL_UpdateTexture(texture, nullptr, emu->get_framebuffer(), tex_w * 4);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    return 0;
}

void free()
{
	delete emu;
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    delete bios;
}

int main(int argc, char** argv)
{
	int err;
	err = init();

	if (!err)
		while (running)
			if (loop())
				err = -1;

	free();
	return err;
}
