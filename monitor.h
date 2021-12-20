#ifndef MONITOR_H
#define MONITOR_H

#include <QWidget>
#include <QRgb>
#include <QPaintEvent>
#include <QTimerEvent>
#include <QPainter>
#include <QImage>
#include <QTime>
#include <QDebug>

class Monitor : public QWidget {
    Q_OBJECT

    int m_WindowWidth, m_WindowHeight;
    QRgb* m_PixelBuffer = nullptr;

protected:
    virtual void paintEvent(QPaintEvent*) override {
        if (!m_PixelBuffer) {
            return;
        }

        QPainter painter(this);
        QImage image((uchar*)m_PixelBuffer, m_WindowWidth, m_WindowHeight, QImage::Format_ARGB32);
        painter.drawImage(0, 0, image);
    }

public:
    Monitor(QWidget *parent = nullptr) : QWidget(parent) {
        this->setWindowTitle(QString("monitor 1"));
    }

    void Draw(QRgb* pixelBuffer, int width, int height) {
        m_WindowWidth = width;
        m_WindowHeight = height;
        m_PixelBuffer = pixelBuffer;
        this->resize(width, height);
        update();
    }

};


#endif // MONITOR_H
