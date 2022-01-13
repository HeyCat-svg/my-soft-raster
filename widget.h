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

#include <windows.h>
#include "geometry.h"
#include "model.h"
#include "shader.h"
#include "light.h"
#include "camera.h"
#include "monitor.h"
#include "accel.h"
#include "world.h"

class SoftRaster : public QWidget {
    Q_OBJECT

private:
    const float Z_MIN = std::numeric_limits<float>::lowest();     // 人为规定z的最小值

    int m_WindowWidth = 600;    // px
    int m_WindowHeight = 600;   // px

    QRgb* m_PixelBuffer = nullptr;  // 像素缓冲 color buffer
    QRgb* m_ShadowMap = nullptr;    //
    QRgb* m_AOMap = nullptr;
    float* m_Zbuffer = nullptr;
    float* m_Zbuffer1 = nullptr;

    int m_RepaintInterval = 5000;    // ms
    int m_RepaintTimer;

    IShader* m_Shader = nullptr;
    IShader* m_ShadowMapShader = nullptr;
    IShader* m_HBAOShader = nullptr;
    IShader* m_ZWriteShader = nullptr;
    IShader* m_RayTracerShader = nullptr;
    IShader* m_PathTracerShader = nullptr;

    Light* m_PointLight = nullptr;
    Camera* m_Camera = nullptr;

    Accel* m_ModelAccel = nullptr;

    Monitor* m_AnotherMonitor = nullptr;      // 用于查看其他buffer画面 如shadow map

protected:
    virtual void paintEvent(QPaintEvent*) override;
    virtual void timerEvent(QTimerEvent* event) override;

public:
    SoftRaster(QWidget *parent = nullptr);
    ~SoftRaster();

    void Line(int x1, int y1, int x2, int y2, QRgb color);  // Bresenham’s Line Drawing Algorithm
    void Triangle(vec4* clipPts, IShader* shader, QRgb* renderTarget, float* zbuffer);                   // 有深度测试 pts.xy是屏幕坐标 pts.z是深度
    vec3 Barycentric(vec2* pts, vec2 p);                    // pts[0]=A pts[1]=B pts[2]=C p=P

};

#endif // WIDGET_H
