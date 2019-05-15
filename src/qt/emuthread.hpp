#ifndef EMUTHREAD_HPP
#define EMUTHREAD_HPP

#include <chrono>

#include <QMutex>
#include <QThread>
#include <string>

#include "../core/emulator.hpp"
#include "../core/errors.hpp"
#include "../common/wsi.hpp"

enum PAUSE_EVENT
{
    FILE_DIALOG,
    MESSAGE_BOX,
    FRAME_ADVANCE,
    USER_REQUESTED
};

class EmuThread : public QThread
{
    Q_OBJECT
    private:
        bool abort;
        uint32_t pause_status;
        QMutex emu_mutex, load_mutex, pause_mutex;
        Emulator e;
        EmuBootSettings current_settings;

        std::chrono::system_clock::time_point old_frametime;
    public:
        EmuThread(EmuBootSettings settings, WindowSystem::Info wsi);
        ~EmuThread();

        bool frame_advance;

        void init();
        bool load_state(const char* name);
        bool save_state(const char* name);

        EmuBootSettings get_current_settings() const;
    protected:
        void run() override;
    signals:
        void completed_frame(uint32_t* buffer, int inner_w, int inner_h, int final_w, int final_h);
        void update_FPS(const QString& FPS);
        void emu_error(QString err);
        void emu_non_fatal_error(QString err);
    public slots:
        void shutdown();
        void press_key(PAD_BUTTON button);
        void release_key(PAD_BUTTON button);
        void update_joystick(JOYSTICK joystick, JOYSTICK_AXIS axis, uint8_t val);
        void pause(PAUSE_EVENT event);
        void unpause(PAUSE_EVENT event);
};

class PauseGuard
{
    private:
        EmuThread* emu_thread;
        PAUSE_EVENT pause_status;
    public:
        PauseGuard(EmuThread* thread, PAUSE_EVENT pause_event);
        ~PauseGuard();
};

#endif // EMUTHREAD_HPP
