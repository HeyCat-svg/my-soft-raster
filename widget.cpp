#include "widget.h"

Model africanHeadModel("./obj/african_head/african_head.obj");

SoftRaster::SoftRaster(QWidget *parent) : QWidget(parent) {
    this->setParent(parent);
    this->setWindowTitle(QString("Soft Raster"));
    this->resize(m_WindowWidth, m_WindowHeight);

    // init pixel buffer
    m_PixelBuffer = new QRgb[m_WindowWidth * m_WindowHeight];

    // start repaint timer
    m_RepaintTimer = startTimer(m_RepaintInterval);
}


SoftRaster::~SoftRaster() {
    killTimer(m_RepaintTimer);
    delete[] m_PixelBuffer;
}


void SoftRaster::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QImage image((uchar*)m_PixelBuffer, m_WindowWidth, m_WindowHeight, QImage::Format_ARGB32);

    // clear image buffer with white
    memset(m_PixelBuffer, ~0, m_WindowWidth * m_WindowHeight * sizeof(QRgb));

//    // lesson 1: draw a line
//    Line(0, 0, 20, 500, (255 << 24) | (255 << 16));    // red

    // lesson 1.1: draw a model wireframe
    QRgb color = (255 << 24) | (255) << 16;     // Red
    int faceCount = africanHeadModel.nfaces();
    for (int i = 0; i < faceCount; ++i) {
        for (int j = 0; j < 3; ++j) {
            vec3 v0 = africanHeadModel.vert(i, j % 3);
            vec3 v1 = africanHeadModel.vert(i, (j + 1) % 3);
            Line(
                0.5f * (v0.x + 1.0f) * m_WindowWidth,
                0.5f * (-v0.y + 1.0f) * m_WindowHeight,     // inverse y
                0.5f * (v1.x + 1.0f) * m_WindowWidth,
                0.5f * (-v1.y + 1.0f) * m_WindowHeight,     // inverse y
                color
            );
        }
    }

    // draw image on window
    painter.drawImage(0, 0, image);
}


void SoftRaster::timerEvent(QTimerEvent* event) {
    int timerID = event->timerId();

    // repaint event
    if (timerID == m_RepaintTimer) {
        update();
    }
}


void SoftRaster::Line(int x1, int y1, int x2, int y2, QRgb color) {
    bool steep = false;     // k<1->false  k>1->true
    // 如果斜率绝对值>1 则交换xy 沿着y考虑x的增长
    if (std::abs(y1 - y2) > std::abs(x1 - x2)) {
        std::swap(x1, y1);
        std::swap(x2, y2);
        steep = true;
    }
    // 必须让x1 < x2
    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    int stepY = (y2 < y1) ? -1 : 1;     // 确定y是向上还是向下长一个像素格

    if (steep) {
        int m = 2 * std::abs(y2 - y1);
        int dx = x2 - x1;
        int error = 0;
        for (int x = x1, y = y1; x <= x2; ++x) {
            m_PixelBuffer[y + x * m_WindowWidth] = color;   // steep=true xy需要交换一下
            error += m;
            if (error > dx) {
                y += stepY;
                error -= 2 * dx;
            }
        }
    }
    else {
        int m = 2 * std::abs(y2 - y1);
        int dx = x2 - x1;
        int error = 0;
        for (int x = x1, y = y1; x <= x2; ++x) {
            m_PixelBuffer[x + y * m_WindowWidth] = color;
            error += m;
            if (error > dx) {
                y += stepY;
                error -= 2 * dx;
            }
        }
    }
}


