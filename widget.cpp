#include "widget.h"

Model africanHeadModel("./obj/african_head/african_head.obj");

SoftRaster::SoftRaster(QWidget *parent) : QWidget(parent) {
    this->setParent(parent);
    this->setWindowTitle(QString("Soft Raster"));
    this->resize(m_WindowWidth, m_WindowHeight);

    // init pixel buffer
    m_PixelBuffer = new QRgb[m_WindowWidth * m_WindowHeight];

    // init zbuffer
    m_Zbuffer = new float[m_WindowWidth * m_WindowHeight];
    for (int i = 0; i < m_WindowHeight; ++i) {
        for (int j = 0; j < m_WindowWidth; ++j) {
            m_Zbuffer[i * m_WindowWidth + j] = Z_MIN;
        }
    }

    // set shader env
    vec3 translate(0, 0, 0);
    vec3 rotation(0, 0, 0);
    vec3 scale(1, 1, 1);
    SetModelMatrix(TRS(translate, rotation, scale));


    // start repaint timer
    m_RepaintTimer = startTimer(m_RepaintInterval);
}


SoftRaster::~SoftRaster() {
    killTimer(m_RepaintTimer);
    delete[] m_PixelBuffer;
    delete[] m_Zbuffer;
}


void SoftRaster::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QImage image((uchar*)m_PixelBuffer, m_WindowWidth, m_WindowHeight, QImage::Format_ARGB32);

    // clear image buffer with white
    memset(m_PixelBuffer, ~0, m_WindowWidth * m_WindowHeight * sizeof(QRgb));

//    // lesson 1: draw a line
//    Line(0, 0, 20, 500, (255 << 24) | (255 << 16));    // red

//    // lesson 1.1: draw a model wireframe
//    QRgb color = (255 << 24) | (255 << 16);     // Red
//    int faceCount = africanHeadModel.nfaces();
//    for (int i = 0; i < faceCount; ++i) {
//        for (int j = 0; j < 3; ++j) {
//            vec3 v0 = africanHeadModel.vert(i, j % 3);
//            vec3 v1 = africanHeadModel.vert(i, (j + 1) % 3);
//            Line(
//                0.5f * (v0.x + 1.0f) * m_WindowWidth,
//                0.5f * (-v0.y + 1.0f) * m_WindowHeight,     // inverse y
//                0.5f * (v1.x + 1.0f) * m_WindowWidth,
//                0.5f * (-v1.y + 1.0f) * m_WindowHeight,     // inverse y
//                color
//            );
//        }
//    }

//    // lesson 2: draw a triangle
//    vec2 pts[3];
//    pts[0] = vec2(0, 0);
//    pts[1] = vec2(300, 100);
//    pts[2] = vec2(100, 300);
//    Triangle(pts, (255 << 24) | (255 << 16));

    QRgb bgColor = 255 << 24;
    for (int i = 0; i < m_WindowHeight; ++i) {
        for (int j = 0; j < m_WindowWidth; ++j) {
            m_PixelBuffer[i * m_WindowWidth + j] = bgColor;
            m_Zbuffer[i * m_WindowWidth + j] = Z_MIN;
        }
    }
    vec3 pts[3];
    int faceCount = africanHeadModel.nfaces();
    for (int i = 0; i < faceCount; ++i) {
        pts[0] = africanHeadModel.vert(i, 0);
        pts[1] = africanHeadModel.vert(i, 1);
        pts[2] = africanHeadModel.vert(i, 2);

        vec3 n = cross(pts[1] - pts[0], pts[2] - pts[0]).normalize();
        float intensity = n * vec3(0, 0, 1);
        // clip when primitive on back
        if (intensity < 0) {
            continue;
        }
        // 转换为屏幕坐标 z值为深度
        vec3 pts2D[3] = {
            0.5f * vec3((pts[0].x + 1) * m_WindowWidth, (-pts[0].y + 1) * m_WindowHeight, pts[0].z),
            0.5f * vec3((pts[1].x + 1) * m_WindowWidth, (-pts[1].y + 1) * m_WindowHeight, pts[1].z),
            0.5f * vec3((pts[2].x + 1) * m_WindowWidth, (-pts[2].y + 1) * m_WindowHeight, pts[2].z)
        };
        Triangle(pts2D, 255 << 24 | ((uint8_t)(255 * intensity) << 16) | ((uint8_t)(255 * intensity) << 8) | (uint8_t)(255 * intensity));
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


void SoftRaster::Triangle(vec2 *pts, QRgb color) {
    vec2 boundBoxMin = vec2(m_WindowWidth - 1, m_WindowHeight - 1);
    vec2 boundBoxMax = vec2(0, 0);

    for (int i = 0; i < 3; ++i) {
        boundBoxMin.x = std::max(0.f, std::min(pts[i].x, boundBoxMin.x));
        boundBoxMin.y = std::max(0.f, std::min(pts[i].y, boundBoxMin.y));

        boundBoxMax.x = std::min(m_WindowWidth - 1.f, std::max(pts[i].x, boundBoxMax.x));
        boundBoxMax.y = std::min(m_WindowHeight - 1.f, std::max(pts[i].y, boundBoxMax.y));
    }

    for (int y = boundBoxMin.y; y <= boundBoxMax.y; ++y) {
        for (int x = boundBoxMin.x; x <= boundBoxMax.x; ++x) {
            vec3 barycentric = Barycentric(pts, vec2(x, y));
            if (barycentric.x < 0.f || barycentric.y < 0.f || barycentric.z < 0.f) {
                continue;
            }

            m_PixelBuffer[x + y * m_WindowWidth] = color;
        }
    }
}


void SoftRaster::Triangle(vec3 *pts, QRgb color) {
    vec2 boundBoxMin = vec2(m_WindowWidth - 1, m_WindowHeight - 1);
    vec2 boundBoxMax = vec2(0, 0);

    for (int i = 0; i < 3; ++i) {
        boundBoxMin.x = std::max(0.f, std::min(pts[i].x, boundBoxMin.x));
        boundBoxMin.y = std::max(0.f, std::min(pts[i].y, boundBoxMin.y));

        boundBoxMax.x = std::min(m_WindowWidth - 1.f, std::max(pts[i].x, boundBoxMax.x));
        boundBoxMax.y = std::min(m_WindowHeight - 1.f, std::max(pts[i].y, boundBoxMax.y));
    }

    for (int y = boundBoxMin.y; y <= boundBoxMax.y; ++y) {
        for (int x = boundBoxMin.x; x <= boundBoxMax.x; ++x) {
            vec2 screenCoords[3] = {vec2(pts[0].x, pts[0].y),
                                    vec2(pts[1].x, pts[1].y),
                                    vec2(pts[2].x, pts[2].y)};
            vec3 barycentric = Barycentric(screenCoords, vec2(x, y));
            if (barycentric.x < 0.f || barycentric.y < 0.f || barycentric.z < 0.f) {
                continue;
            }
            float depth = barycentric.x * pts[0].z + barycentric.y * pts[1].z + barycentric.z * pts[2].z;
            // 深度测试 z从里到外增大
            if (depth > m_Zbuffer[x + y * m_WindowWidth]) {
                m_Zbuffer[x + y * m_WindowWidth] = depth;
                m_PixelBuffer[x + y * m_WindowWidth] = color;
            }
        }
    }
}


// 解重心坐标 u*AB + v*AC + PA = 0
vec3 SoftRaster::Barycentric(vec2 *pts, vec2 p) {
    vec3 ret = cross(
        vec3(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - p.x),
        vec3(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - p.y)
    );

    // 整数坐标输入 cross后应该也是整数 abs(ret.z)<1意味着ret[2]是0 即输入三角形退化成线段或点
    if (std::abs(ret.z) < 1.f) {
        return vec3(-1.f, 1.f, 1.f);
    }
    // (ret.x + ret.y) / ret.z 先+后x 增加精度 避免在BC边上像素漏画
    return vec3(1.f - (ret.x + ret.y) / ret.z, ret.x / ret.z, ret.y / ret.z);
}


