#include <cmath>
#include <fstream>
#include <iostream>

#include <QApplication>
#include <QString>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTableWidget>
#include <QDebug>

#include "emuwindow.hpp"
#include "settingswindow.hpp"
#include "renderwidget.hpp"
#include "gamelistwidget.hpp"
#include "bios.hpp"

EmuWindow::EmuWindow(QWidget *parent) 
    : QMainWindow(parent), emu_thread(nullptr), settings_window(nullptr)
{
    old_frametime = std::chrono::system_clock::now();
    old_update_time = std::chrono::system_clock::now();
    framerate_avg = 0.0;

    render_widget = new RenderWidget;

    GameListWidget* game_list_widget = new GameListWidget;
    connect(game_list_widget, &GameListWidget::game_double_clicked, this, [=](QString path) {
        boot(path, true);
    });
    connect(game_list_widget, &GameListWidget::settings_requested, [=]() {
        open_settings_window();
        settings_window->show_path_tab();
    });

    stack_widget = new QStackedWidget;
    stack_widget->addWidget(game_list_widget);
    stack_widget->addWidget(render_widget);
    stack_widget->setMinimumWidth(RenderWidget::DEFAULT_WIDTH);
    stack_widget->setMinimumHeight(RenderWidget::DEFAULT_HEIGHT);

    setCentralWidget(stack_widget);

    create_menu();

    //Initialize window
    show_default_view();
    show();
}

void EmuWindow::boot(QString file, bool fast)
{
    if (emu_thread)
    {
        emu_thread->shutdown();
        emu_thread->wait();
    }

    WindowSystem::Info wsi = {};
    wsi.render_surface = render_widget->get_native_handle();
#ifdef WIN32
    wsi.type = WindowSystem::Type::DWM;
#endif;

    BiosReader bios_file(Settings::instance().bios_path);
    if (!bios_file.is_valid())
    {
        emu_error("Bios file is not valid");
        return;
    }

    QFileInfo file_info(file);
    if (!file_info.exists())
    {
        emu_error("ROM file does not exist");
        return;
    }

    EmuBootSettings settings = {};
    settings.bios = bios_file.data();
    settings.vu1_jit = Settings::instance().vu1_jit_enabled;
    settings.fast_boot = fast;
    settings.rom_path = file.toStdString();

    emu_thread = new EmuThread(settings, wsi);

    connect(this, &EmuWindow::press_key, emu_thread, &EmuThread::press_key);
    connect(this, &EmuWindow::release_key, emu_thread, &EmuThread::release_key);
    connect(this, &EmuWindow::update_joystick, emu_thread, &EmuThread::update_joystick);

    connect(emu_thread, &EmuThread::update_FPS, this, &EmuWindow::update_FPS);
    connect(emu_thread, &EmuThread::emu_error, this, &EmuWindow::emu_error);
    connect(emu_thread, &EmuThread::emu_non_fatal_error, this, &EmuWindow::emu_non_fatal_error);
    connect(emu_thread, &QThread::started, this, &EmuWindow::show_render_view);
    connect(emu_thread, &QThread::finished, this, &EmuWindow::show_default_view);

    // TODO: Remove
    connect(emu_thread, &EmuThread::completed_frame, render_widget, &RenderWidget::draw_frame);

    // Don't delete the QThread until after it's finished.
    // Not doing so will cause a crash.
    connect(emu_thread, &QThread::finished, emu_thread, &QObject::deleteLater);

    emu_thread->init();
    emu_thread->start();
}

void EmuWindow::create_menu()
{
    load_rom_action = new QAction(tr("Load ROM... (&Fast)"), this);
    connect(load_rom_action, &QAction::triggered, this, &EmuWindow::open_file_skip);

    load_bios_action = new QAction(tr("Load ROM... (&Boot BIOS)"), this);
    connect(load_bios_action, &QAction::triggered, this, &EmuWindow::open_file_no_skip);

    load_state_action = new QAction(tr("&Load State"), this);
    connect(load_state_action, &QAction::triggered, this, &EmuWindow::load_state);

    save_state_action = new QAction(tr("&Save State"), this);
    connect(save_state_action, &QAction::triggered, this, &EmuWindow::save_state);

    exit_action = new QAction(tr("&Exit"), this);
    connect(exit_action, &QAction::triggered, this, &QWidget::close);

    file_menu = menuBar()->addMenu(tr("&File"));
    file_menu->addAction(load_rom_action);
    file_menu->addAction(load_bios_action);

    auto recent_menu = file_menu->addMenu(tr("&Recent"));
    auto default_action = new QAction(tr("No recent roms..."));
    default_action->setEnabled(false);

    if (Settings::instance().recent_roms.isEmpty())
    {
        recent_menu->addAction(default_action);
    }

    for (auto& recent_file : Settings::instance().recent_roms)
    {
        auto recent_item_action = new QAction(recent_file);
        connect(recent_item_action, &QAction::triggered, this, [=]() {
            boot(recent_file, true);
        });
        recent_menu->addAction(recent_item_action);
    }

    connect(&Settings::instance(), &Settings::rom_path_added, this, [=](QString path) {
        auto new_action = new QAction(path);
        auto top_action = recent_menu->actions().first();

        connect(new_action, &QAction::triggered, this, [=]() {
            boot(path, true);
        });

        recent_menu->insertAction(top_action, new_action);

        if (recent_menu->actions().contains(default_action))
            recent_menu->removeAction(default_action);
    });

    recent_menu->addSeparator();

    auto clear_action = new QAction(tr("Clear List"));

    connect(clear_action, &QAction::triggered, this, [=]() {
        Settings::instance().clear_rom_paths();
        for (auto& old_action : recent_menu->actions())
        {
            recent_menu->removeAction(old_action);
        }

        recent_menu->addAction(default_action);
        recent_menu->addSeparator();
        recent_menu->addAction(clear_action);
    });

    recent_menu->addAction(clear_action);

    file_menu->addMenu(recent_menu);
    file_menu->addSeparator();
    file_menu->addAction(load_state_action);
    file_menu->addAction(save_state_action);
    file_menu->addSeparator();
    file_menu->addAction(exit_action);

    auto pause_action = new QAction(tr("&Pause"), this);
    connect(pause_action, &QAction::triggered, this, [=] (){
        if (!emu_thread)
            return;

        emu_thread->pause(PAUSE_EVENT::USER_REQUESTED);
    });

    auto unpause_action = new QAction(tr("&Unpause"), this);
    connect(unpause_action, &QAction::triggered, this, [=] (){
        if (!emu_thread)
            return;

        emu_thread->unpause(PAUSE_EVENT::USER_REQUESTED);
    });

    auto frame_action = new QAction(tr("&Frame Advance"), this);
    frame_action->setCheckable(true);
    connect(frame_action, &QAction::triggered, this, [=] (){
        if (!emu_thread)
            return;

        emu_thread->frame_advance ^= true;

        if(!emu_thread->frame_advance)
            emu_thread->unpause(PAUSE_EVENT::FRAME_ADVANCE);

        frame_action->setChecked(emu_thread->frame_advance);
    });

    auto shutdown_action = new QAction(tr("&Shutdown"), this);
    connect(shutdown_action, &QAction::triggered, this, [=]() {
        if (emu_thread)
            emu_thread->shutdown();
    });

    emulation_menu = menuBar()->addMenu(tr("Emulation"));
    emulation_menu->addAction(pause_action);
    emulation_menu->addAction(unpause_action);
    emulation_menu->addSeparator();
    emulation_menu->addAction(frame_action);
    emulation_menu->addSeparator();
    emulation_menu->addAction(shutdown_action);

    auto settings_action = new QAction(tr("&Settings"), this);
    connect(settings_action, &QAction::triggered, this, &EmuWindow::open_settings_window);

    options_menu = menuBar()->addMenu(tr("&Options"));
    options_menu->addAction(settings_action);

    auto ignore_aspect_ratio_action =
    new QAction(tr("&Ignore aspect ratio"), this);
    ignore_aspect_ratio_action->setCheckable(true);

    connect(ignore_aspect_ratio_action, &QAction::triggered, render_widget, [=] (){
        render_widget->toggle_aspect_ratio();
        ignore_aspect_ratio_action->setChecked(
            !render_widget->get_respect_aspect_ratio()
        );
    });

    window_menu = menuBar()->addMenu(tr("&Window"));
    window_menu->addAction(ignore_aspect_ratio_action);
    window_menu->addSeparator();

    for (int factor = 1; factor <= RenderWidget::MAX_SCALING; factor++)
    {
        auto scale_action = new QAction(
            QString("Scale &%1x").arg(factor), this
        );

        connect(scale_action, &QAction::triggered, this, [=]() {
            // Force the widget to the new size
            stack_widget->setMinimumSize(
                RenderWidget::DEFAULT_WIDTH * factor,
                RenderWidget::DEFAULT_HEIGHT * factor
            );

            showNormal();
            adjustSize();

            // reset it so the user can resize the window
            // normally
            stack_widget->setMinimumSize(
                RenderWidget::DEFAULT_WIDTH,
                RenderWidget::DEFAULT_HEIGHT
            );
        });

        window_menu->addAction(scale_action);
    }

    auto screenshot_action = new QAction(tr("&Take Screenshot"), this);
    connect(screenshot_action, &QAction::triggered, render_widget, &RenderWidget::screenshot);

    window_menu->addSeparator();
    window_menu->addAction(screenshot_action);
}

void EmuWindow::open_settings_window()
{
    if (!settings_window)
        settings_window = new SettingsWindow(this);

    settings_window->show();
    settings_window->raise();
}

void EmuWindow::closeEvent(QCloseEvent *event)
{
    emit shutdown();
    event->accept();
}

void EmuWindow::keyPressEvent(QKeyEvent *event)
{
    event->accept();
    switch (event->key())
    {
        case Qt::Key_Up:
            emit press_key(PAD_BUTTON::UP);
            break;
        case Qt::Key_Down:
            emit press_key(PAD_BUTTON::DOWN);
            break;
        case Qt::Key_Left:
            emit press_key(PAD_BUTTON::LEFT);
            break;
        case Qt::Key_Right:
            emit press_key(PAD_BUTTON::RIGHT);
            break;
        case Qt::Key_Z:
            emit press_key(PAD_BUTTON::CIRCLE);
            break;
        case Qt::Key_X:
            emit press_key(PAD_BUTTON::CROSS);
            break;
        case Qt::Key_A:
            emit press_key(PAD_BUTTON::TRIANGLE);
            break;
        case Qt::Key_S:
            emit press_key(PAD_BUTTON::SQUARE);
            break;
        case Qt::Key_Q:
            emit press_key(PAD_BUTTON::L1);
            break;
        case Qt::Key_W:
            emit press_key(PAD_BUTTON::R1);
            break;
        case Qt::Key_Return:
            emit press_key(PAD_BUTTON::START);
            break;
        case Qt::Key_Space:
            emit press_key(PAD_BUTTON::SELECT);
            break;
        case Qt::Key_Period:
            emu_thread->unpause(PAUSE_EVENT::FRAME_ADVANCE);
            break;
        case Qt::Key_J:
            emit update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::X, 0x00);
            break;
        case Qt::Key_L:
            emit update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::X, 0xFF);
            break;
        case Qt::Key_I:
            emit update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::Y, 0x00);
            break;
        case Qt::Key_K:
            emit update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::Y, 0xFF);
            break;
        case Qt::Key_F8:
            render_widget->screenshot();
            break;
    }
}

void EmuWindow::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
    switch (event->key())
    {
        case Qt::Key_Up:
            emit release_key(PAD_BUTTON::UP);
            break;
        case Qt::Key_Down:
            emit release_key(PAD_BUTTON::DOWN);
            break;
        case Qt::Key_Left:
            emit release_key(PAD_BUTTON::LEFT);
            break;
        case Qt::Key_Right:
            emit release_key(PAD_BUTTON::RIGHT);
            break;
        case Qt::Key_Z:
            emit release_key(PAD_BUTTON::CIRCLE);
            break;
        case Qt::Key_X:
            emit release_key(PAD_BUTTON::CROSS);
            break;
        case Qt::Key_A:
            emit release_key(PAD_BUTTON::TRIANGLE);
            break;
        case Qt::Key_S:
            emit release_key(PAD_BUTTON::SQUARE);
            break;
        case Qt::Key_Q:
            emit release_key(PAD_BUTTON::L1);
            break;
        case Qt::Key_W:
            emit release_key(PAD_BUTTON::R1);
            break;
        case Qt::Key_Return:
            emit release_key(PAD_BUTTON::START);
            break;
        case Qt::Key_Space:
            emit release_key(PAD_BUTTON::SELECT);
            break;
        case Qt::Key_J:
        case Qt::Key_L:
            emit update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::X, 0x80);
            break;
        case Qt::Key_K:
        case Qt::Key_I:
            emit update_joystick(JOYSTICK::LEFT, JOYSTICK_AXIS::Y, 0x80);
            break;
    }
}

void EmuWindow::update_FPS(const QString& FPS)
{
    using namespace std::chrono;
    // average framerate over 1 second
    system_clock::time_point now = system_clock::now();
    duration<double> elapsed_update_seconds = now - old_update_time;

    if (elapsed_update_seconds.count() >= 0.5)
    {
        EmuBootSettings settings = emu_thread->get_current_settings();
        QFileInfo rom_info(QString::fromStdString(settings.rom_path));

        QString status = QString("%1 FPS - %2 [VU1: %3] [Vulkan Software]")
            .arg(FPS)
            .arg(rom_info.fileName())
            .arg(QString::fromStdString(settings.vu_mode_string()));

        setWindowTitle(status);
        old_update_time = system_clock::now();
    }
}

void EmuWindow::bios_error(QString err)
{
    QMessageBox msg_box;
    msg_box.setText("Emulation has been terminated");
    msg_box.setInformativeText(err);
    msg_box.setStandardButtons(QMessageBox::Abort);
    msg_box.setDefaultButton(QMessageBox::Abort);
    msg_box.exec();
}

void EmuWindow::emu_error(QString err)
{
    QMessageBox msgBox;
    msgBox.setText("Emulation has been terminated");
    msgBox.setInformativeText(err);
    msgBox.setStandardButtons(QMessageBox::Abort);
    msgBox.setDefaultButton(QMessageBox::Abort);
    msgBox.exec();
}

void EmuWindow::emu_non_fatal_error(QString err)
{
    QMessageBox msgBox;
    msgBox.setText("Error");
    msgBox.setInformativeText(err);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.setDefaultButton(QMessageBox::Ok);
    msgBox.exec();
    emu_thread->unpause(MESSAGE_BOX);
}

#ifndef QT_NO_CONTEXTMENU
void EmuWindow::contextMenuEvent(QContextMenuEvent* event)
{
    QMenu menu(this);
    menu.addAction(load_rom_action);
    menu.addAction(load_bios_action);
    menu.addAction(exit_action);
    menu.exec(event->globalPos());
}
#endif // QT_NO_CONTEXTMENU

void EmuWindow::open_file_no_skip()
{
    PauseGuard pg(emu_thread, PAUSE_EVENT::FILE_DIALOG);

    QString file_name = QFileDialog::getOpenFileName(
        this, tr("Open Rom"), Settings::instance().last_used_directory,
        tr("ROM Files (*.elf *.iso *.cso)")
    );

    if (!file_name.isEmpty())
    {
        Settings::instance().add_rom_path(file_name);
        boot(file_name, false);
    }
}

void EmuWindow::open_file_skip()
{
    PauseGuard pg(emu_thread, PAUSE_EVENT::FILE_DIALOG);

    QString file_name = QFileDialog::getOpenFileName(
        this, tr("Open Rom"), Settings::instance().last_used_directory,
        tr("ROM Files (*.elf *.iso *.cso)")
    );

    if (!file_name.isEmpty())
    {
        Settings::instance().add_rom_path(file_name);
        boot(file_name, true);
    }
}

void EmuWindow::load_state()
{
    if (!emu_thread)
        return;

    PauseGuard pg(emu_thread, PAUSE_EVENT::FILE_DIALOG);

    EmuBootSettings settings = emu_thread->get_current_settings();

    QFileInfo rom_info(QString::fromStdString(settings.rom_path));
    QString directory = rom_info.absoluteDir().path();

    QString save_state = directory
        .append(QDir::separator())
        .append(rom_info.fileName())
        .append(".snp");

    if (!emu_thread->load_state(save_state.toLocal8Bit()))
        printf("Failed to load %s\n", qPrintable(save_state));
}

void EmuWindow::save_state()
{
    if (!emu_thread)
        return;

    PauseGuard pg(emu_thread, PAUSE_EVENT::FILE_DIALOG);
    
    EmuBootSettings settings = emu_thread->get_current_settings();

    QFileInfo rom_info(QString::fromStdString(settings.rom_path));
    QString directory = rom_info.absoluteDir().path();

    QString save_state = directory
        .append(QDir::separator())
        .append(rom_info.fileName())
        .append(".snp");

    if (!emu_thread->save_state(save_state.toLocal8Bit()))
        printf("Failed to save %s\n", qPrintable(save_state));
}

void EmuWindow::show_default_view()
{
    stack_widget->setCurrentIndex(0);
    setWindowTitle(QApplication::applicationName());
}

void EmuWindow::show_render_view()
{
    stack_widget->setCurrentIndex(1);
}