#ifndef RENDERWIDGET_HPP
#define RENDERWIDGET_HPP

#include <QWidget>
#include <QPaintEvent>

#ifdef __linux__
#include <xcb/xcb.h>
#include <QApplication>
#include <qpa/qplatformnativeinterface.h>
#endif

class RenderWidget : public QWidget
{
    Q_OBJECT
    private:
        QImage final_image;
        bool respect_aspect_ratio = true;
    public:
        static const int MAX_SCALING = 4;
        static const int DEFAULT_WIDTH = 640;
        static const int DEFAULT_HEIGHT = 448;

        explicit RenderWidget(QWidget* parent = nullptr);
        void paintEvent(QPaintEvent* event) override;

        bool get_respect_aspect_ratio() const;
        void* handle();
#ifdef __linux__
        xcb_connection_t* connection();
#endif
    public slots:
        void draw_frame(uint32_t* buffer, int inner_w, int inner_h, int final_w, int final_h);
        void toggle_aspect_ratio();
        void screenshot();
};
#endif