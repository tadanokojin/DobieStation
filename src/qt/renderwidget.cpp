#include <QPainter>
#include <QImageWriter>
#include <QDateTime>
#include <QDebug>
#include <QWindow>

#include "renderwidget.hpp"
#include "settings.hpp"

RenderWidget::RenderWidget(QWidget* parent)
    : QWidget(parent)
{
    QPalette palette;
    palette.setColor(QPalette::Background, Qt::black);
    setPalette(palette);
    setAutoFillBackground(true);

    // We need a native window to render to
    setAttribute(Qt::WA_NativeWindow);
}

void* RenderWidget::get_native_handle() const
{
    return reinterpret_cast<void*>(
        windowHandle()->winId()
    );
}