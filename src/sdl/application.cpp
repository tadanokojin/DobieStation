#include "application.hpp"
#include <SDL.h>


Application::Application() :
    running(false), fps_counter(0),
    pad(nullptr), joy_id(-1)
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

    next_tick = high_resolution_clock::now() + sec;
    return running = true;
}

void Application::free()
{
    SDL_GameControllerClose(pad);
    window.close();
    SDL_Quit();
}

bool Application::frame()
{
    // fps counter
    if (high_resolution_clock::now() >= next_tick)
    {
        char window_title[256];
        snprintf(window_title, sizeof(window_title), "%li FPS", fps_counter);
        window.set_title(window_title);

        next_tick += sec;
        fps_counter = 0;
    }

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
    ++fps_counter;

    return true;
}


void Application::key_event(int32_t keycode, bool down)
{
    auto button = PAD_BUTTON::NONE;
    auto axis   = JOYSTICK_AXIS::NONE;
    uint8_t axis_dir = 0;

    switch (keycode)
    {
    case (SDLK_a):      button = PAD_BUTTON::TRIANGLE; break;
    case (SDLK_s):      button = PAD_BUTTON::SQUARE;   break;
    case (SDLK_z):      button = PAD_BUTTON::CIRCLE;   break;
    case (SDLK_x):      button = PAD_BUTTON::CROSS;    break;
    case (SDLK_RETURN): button = PAD_BUTTON::START;    break;
    case (SDLK_LSHIFT): button = PAD_BUTTON::SELECT;   break;
    case (SDLK_q):      button = PAD_BUTTON::L1;       break;
    case (SDLK_w):      button = PAD_BUTTON::R1;       break;
    case (SDLK_UP):     button = PAD_BUTTON::UP;       break;
    case (SDLK_DOWN):   button = PAD_BUTTON::DOWN;     break;
    case (SDLK_LEFT):   button = PAD_BUTTON::LEFT;     break;
    case (SDLK_RIGHT):  button = PAD_BUTTON::RIGHT;    break;

    case (SDLK_i): axis = JOYSTICK_AXIS::Y; axis_dir = 0x00; break;
    case (SDLK_k): axis = JOYSTICK_AXIS::Y; axis_dir = 0xFF; break;
    case (SDLK_j): axis = JOYSTICK_AXIS::X; axis_dir = 0x00; break;
    case (SDLK_l): axis = JOYSTICK_AXIS::X; axis_dir = 0xFF; break;

    default: break;
    }

    if (button != PAD_BUTTON::NONE)
        emu.update_button(button, down ? 0xFF : 0x00);
    else if (axis != JOYSTICK_AXIS::NONE)
        emu.update_joystick(JOYSTICK::LEFT, axis, down ? axis_dir : 0x7F);
}

void Application::button_event(int button, bool down)
{
    auto ds_button = PAD_BUTTON::NONE;

    switch (button)
    {
    case (SDL_CONTROLLER_BUTTON_B): ds_button = PAD_BUTTON::CIRCLE;   break;
    case (SDL_CONTROLLER_BUTTON_A): ds_button = PAD_BUTTON::CROSS;    break;
    case (SDL_CONTROLLER_BUTTON_Y): ds_button = PAD_BUTTON::TRIANGLE; break;
    case (SDL_CONTROLLER_BUTTON_X): ds_button = PAD_BUTTON::SQUARE;   break;

    case (SDL_CONTROLLER_BUTTON_START): ds_button = PAD_BUTTON::START;  break;
    case (SDL_CONTROLLER_BUTTON_BACK):  ds_button = PAD_BUTTON::SELECT; break;

    case (SDL_CONTROLLER_BUTTON_LEFTSHOULDER):  ds_button = PAD_BUTTON::L1; break;
    case (SDL_CONTROLLER_BUTTON_RIGHTSHOULDER): ds_button = PAD_BUTTON::R1; break;
    case (SDL_CONTROLLER_BUTTON_LEFTSTICK):  ds_button = PAD_BUTTON::L3; break;
    case (SDL_CONTROLLER_BUTTON_RIGHTSTICK): ds_button = PAD_BUTTON::R3; break;

    case (SDL_CONTROLLER_BUTTON_DPAD_UP):    ds_button = PAD_BUTTON::UP;    break;
    case (SDL_CONTROLLER_BUTTON_DPAD_DOWN):  ds_button = PAD_BUTTON::DOWN;  break;
    case (SDL_CONTROLLER_BUTTON_DPAD_LEFT):  ds_button = PAD_BUTTON::LEFT;  break;
    case (SDL_CONTROLLER_BUTTON_DPAD_RIGHT): ds_button = PAD_BUTTON::RIGHT; break;

    default: break;
    }

    if (ds_button != PAD_BUTTON::NONE)
        emu.update_button(ds_button, down ? 0xFF : 0x00);
}

void Application::joystick_event(int axis, int16_t value)
{
    auto ds_val   = (uint8_t)((uint16_t)(value + 0x8000) >> 8U);
    auto trig_val = (uint8_t)((uint16_t)value >> 7U);

    if (axis == SDL_CONTROLLER_AXIS_LEFTX)
        emu.update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::X, ds_val);
    else if (axis == SDL_CONTROLLER_AXIS_LEFTY)
        emu.update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::Y, ds_val);
    else if (axis == SDL_CONTROLLER_AXIS_RIGHTX)
        emu.update_joystick(JOYSTICK::RIGHT, JOYSTICK_AXIS::X, ds_val);
    else if (axis == SDL_CONTROLLER_AXIS_RIGHTY)
        emu.update_joystick(JOYSTICK::RIGHT, JOYSTICK_AXIS::Y, ds_val);
    else if (axis == SDL_CONTROLLER_AXIS_TRIGGERLEFT)
        emu.update_button(PAD_BUTTON::L2, trig_val);
    else if (axis == SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        emu.update_button(PAD_BUTTON::R2, trig_val);
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

    case (SDL_KEYDOWN):
        key_event(event.key.keysym.sym, true);
        break;

    case (SDL_KEYUP):
        key_event(event.key.keysym.sym, false);
        break;

    case (SDL_CONTROLLERDEVICEADDED):
        if (!pad && (pad = SDL_GameControllerOpen(event.cdevice.which)))
        {
            joy_id = event.cdevice.which;
            fprintf(stderr, "Using gamepad #%d, \"%s\"\n as Dualshock #1.", joy_id, SDL_GameControllerName(pad));
        }
        break;

    case (SDL_CONTROLLERDEVICEREMOVED):
        if (pad && event.cdevice.which == joy_id)
        {
            pad = nullptr;
            joy_id = -1;
            fprintf(stderr, "Dualshock #1 removed.");
        }
        break;

    case (SDL_CONTROLLERAXISMOTION):
        if (event.caxis.which == joy_id)
            joystick_event(event.caxis.axis, event.caxis.value);
        break;

    case (SDL_CONTROLLERBUTTONDOWN):
    case (SDL_CONTROLLERBUTTONUP):
        if (event.cbutton.which == joy_id)
            button_event(event.cbutton.button, event.cbutton.state == SDL_PRESSED);
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
