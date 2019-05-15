#include <cmath>
#include <chrono>
#include <fstream>
#include <QDebug>

#include "emuthread.hpp"

EmuThread::EmuThread(EmuBootSettings settings, WindowSystem::Info wsi)
    : abort(false), pause_status(0x0), frame_advance(false), 
    current_settings(settings), e(settings, wsi)
{
    qDebug() << this;
    qDebug() << "RAM OK";
    qDebug() << "ROM OK";
    qDebug() << "Boot Settings:";
    qDebug() << "\tROM:\t\t" << settings.rom_path.c_str();
    qDebug() << "\tVU:\t\t" << QString::fromStdString(settings.vu_mode_string());
    qDebug() << "\tFast Boot:\t" << settings.fast_boot;
    qDebug() << "Window Settings:";
    qDebug() << "\tSurface ptr:\t" << wsi.render_surface;
}

EmuThread::~EmuThread()
{
    qDebug() << "Emulation Thread Destroyed";
}

void EmuThread::init()
{
    try
    {
        e.load_BIOS(current_settings);
        e.load_ROM(current_settings);
    }
    catch (Emulation_error &error)
    {
        // order matters
        abort = true;
        emit emu_error(error.what());
    }
}

bool EmuThread::load_state(const char *name)
{
    load_mutex.lock();
    bool fail = false;
    if (!e.request_load_state(name))
        fail = true;
    load_mutex.unlock();
    return fail;
}

bool EmuThread::save_state(const char *name)
{
    load_mutex.lock();
    bool fail = false;
    if (!e.request_save_state(name))
        fail = true;
    load_mutex.unlock();
    return fail;
}

void EmuThread::run()
{
    forever
    {
        QMutexLocker locker(&emu_mutex);
        if (abort)
            return;
        else if (pause_status)
            usleep(10000);
        else
        {
            if (frame_advance)
                pause(PAUSE_EVENT::FRAME_ADVANCE);
            try
            {
                e.run();
                int w, h, new_w, new_h;
                e.get_inner_resolution(w, h);
                e.get_resolution(new_w, new_h);
                emit completed_frame(e.get_framebuffer(), w, h, new_w, new_h);

                using namespace std::chrono;

                system_clock::time_point now = system_clock::now();
                duration<double> elapsed_seconds = now - old_frametime;

                double FPS = 1 / elapsed_seconds.count();

                old_frametime = system_clock::now();
                emit update_FPS(QString::number(FPS, 'f', 2));
            }
            catch (non_fatal_error &error)
            {
                printf("non_fatal emulation error occurred\n%s\n", error.what());
                emit emu_non_fatal_error(QString(error.what()));
                pause(PAUSE_EVENT::MESSAGE_BOX);
            }
            catch (Emulation_error &error)
            {
                e.print_state();
                printf("Fatal emulation error occurred, stopping execution\n%s\n", error.what());
                fflush(stdout);
                emit emu_error(QString(error.what()));
                return;
            }
        }
    }
}

void EmuThread::shutdown()
{
    pause_mutex.lock();
    abort = true;
    pause_mutex.unlock();
}

void EmuThread::press_key(PAD_BUTTON button)
{
    pause_mutex.lock();
    e.press_button(button);
    pause_mutex.unlock();
}

void EmuThread::release_key(PAD_BUTTON button)
{
    pause_mutex.lock();
    e.release_button(button);
    pause_mutex.unlock();
}

void EmuThread::update_joystick(JOYSTICK joystick, JOYSTICK_AXIS axis, uint8_t val)
{
    pause_mutex.lock();
    e.update_joystick(joystick, axis, val);
    pause_mutex.unlock();
}

void EmuThread::pause(PAUSE_EVENT event)
{
    pause_status |= 1 << event;
}

void EmuThread::unpause(PAUSE_EVENT event)
{
    pause_status &= ~(1 << event);
}

EmuBootSettings EmuThread::get_current_settings() const
{
    return current_settings;
}

PauseGuard::PauseGuard(EmuThread* thread, PAUSE_EVENT pause)
    : emu_thread(thread), pause_status(pause)
{
    if (emu_thread)
        emu_thread->pause(pause_status);
}

PauseGuard::~PauseGuard()
{
    if (emu_thread)
        emu_thread->unpause(pause_status);
}
