#ifndef WIDGET_H
#define WIDGET_H

#include <iostream>
#include <QWidget>
#include <QRgb>
#include <QPaintEvent>
#include <QTimerEvent>
#include <QPainter>
#include <QImage>
#include <QTime>
#include <QDebug>

#include "geometry.h"
#include "model.h"

class SoftRaster : public QWidget {
    Q_OBJECT

private:
    int m_WindowWidth = 600;    // px
    int m_WindowHeight = 600;   // px

    QRgb* m_PixelBuffer = nullptr;  // 像素缓冲

    int m_RepaintInterval = 100;    // ms
    int m_RepaintTimer;

protected:
    virtual void paintEvent(QPaintEvent*) override;
    virtual void timerEvent(QTimerEvent* event) override;

public:
    SoftRaster(QWidget *parent = nullptr);
    ~SoftRaster();

    void Line(int x1, int y1, int x2, int y2, QRgb color);  // Bresenham’s Line Drawing Algorithm
    void Triangle(vec2* pts, QRgb color);                   // 使用重心坐标判断AABB包围盒内像素是否属于三角形内
    vec3 Barycentric(vec2* pts, vec2 p);                    // pts[0]=A pts[1]=B pts[2]=C p=P

};

#endif // WIDGET_H
