#ifndef RENDERWIDGET_HPP
#define RENDERWIDGET_HPP

#include <QWidget>
#include <QPaintEvent>

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

        void* get_native_handle() const;
};
#endif