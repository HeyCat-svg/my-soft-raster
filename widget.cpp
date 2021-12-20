#include "widget.h"

Model africanHeadModel("./obj/diablo3_pose/diablo3_pose.obj");

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
    // 设置模型TRS
    vec3 translate(0, 0, 0);
    vec3 rotation(0, 0, 0);
    vec3 scale(1.2, 1.2, 1.2);
    mat4x4 model = TRS(translate, rotation, scale);
    SetModelMatrix(model);

    // 设置相机view矩阵
    vec3 worldUp(0, 1, 0);
    vec3 cameraPos(0.5f, 0.5f, 2.5f);
    vec3 lookDir = vec3(0, 0, 0) - cameraPos;
    mat4x4 lookAt = LookAt(lookDir, worldUp);
    SetViewMatrix(cameraPos, lookAt);

    // 设置相机投影参数

    mat4x4 proj = PerspProjection(PI / 3.f, 1.f, 0.3f, 10.f);
    SetProjectionMatrix(proj);

    // 设置光源
    vec4 light(1, 1, 1, 1);
    SetCameraAndLight(cameraPos, light);

    // 加载资源与生成shader
    TGAImage* diffuseImg = new TGAImage();
    diffuseImg->read_tga_file("./obj/diablo3_pose/diablo3_pose_diffuse.tga");
    diffuseImg->flip_vertically();      // 垂直反转让uv取到正确的值
    TGAImage* normalImg = new TGAImage();
    normalImg->read_tga_file("./obj/diablo3_pose/diablo3_pose_nm_tangent.tga");
    normalImg->flip_vertically();
    TGAImage* specImg = new TGAImage();
    specImg->read_tga_file("./obj/diablo3_pose/diablo3_pose_spec.tga");
    specImg->flip_vertically();
    m_Shader = new GeneralShader(&africanHeadModel, diffuseImg, normalImg, specImg);

    // start repaint timer
    m_RepaintTimer = startTimer(m_RepaintInterval);
}


SoftRaster::~SoftRaster() {
    killTimer(m_RepaintTimer);
    delete[] m_PixelBuffer;
    delete[] m_Zbuffer;
    delete m_Shader;
    delete m_ShadowMapShader;
    delete m_PointLight;
    delete m_Camera;
}


void SoftRaster::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QImage image((uchar*)m_PixelBuffer, m_WindowWidth, m_WindowHeight, QImage::Format_ARGB32);

    LARGE_INTEGER cpuFreq;
    LARGE_INTEGER startTime;
    LARGE_INTEGER endTime;
    double runtime = 0.0;
    QueryPerformanceFrequency(&cpuFreq);
    QueryPerformanceCounter(&startTime);

    // clear image with black and clear depth buffer
    QRgb bgColor = 255 << 24;
#pragma omp parallel for
    for (int i = 0; i < m_WindowHeight; ++i) {
        for (int j = 0; j < m_WindowWidth; ++j) {
            m_PixelBuffer[i * m_WindowWidth + j] = bgColor;
            m_Zbuffer[i * m_WindowWidth + j] = Z_MIN;
        }
    }

    // Pass 0: render shader map
    {

    }

    // Pass 1: render model
    {
        int faceCount = africanHeadModel.nfaces();
        for (int i = 0; i < faceCount; ++i) {
            vec4 clipPts[3];
            for (int j = 0; j < 3; ++j) {
                clipPts[j] = m_Shader->Vertex(i, j);
            }
            m_Shader->Geometry();
            Triangle(clipPts, m_Shader);
        }
    }


    // draw image on window
    painter.drawImage(0, 0, image);

    QueryPerformanceCounter(&endTime);
    runtime = (((endTime.QuadPart - startTime.QuadPart) * 1000.0f) / cpuFreq.QuadPart);
    qDebug() << "runtime: " << runtime << "ms";
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


void SoftRaster::Triangle(vec4 *clipPts, IShader* shader) {
    vec2 screenPts[3] = {
        vec2((0.5f * (clipPts[0].x / clipPts[0].w) + 0.5f) * m_WindowWidth, (0.5f * (clipPts[0].y / clipPts[0].w) + 0.5f) * m_WindowHeight),
        vec2((0.5f * (clipPts[1].x / clipPts[1].w) + 0.5f) * m_WindowWidth, (0.5f * (clipPts[1].y / clipPts[1].w) + 0.5f) * m_WindowHeight),
        vec2((0.5f * (clipPts[2].x / clipPts[2].w) + 0.5f) * m_WindowWidth, (0.5f * (clipPts[2].y / clipPts[2].w) + 0.5f) * m_WindowHeight)
    };

    vec2 boundBoxMin = vec2(m_WindowWidth - 1, m_WindowHeight - 1);
    vec2 boundBoxMax = vec2(0, 0);

    for (int i = 0; i < 3; ++i) {
        boundBoxMin.x = std::max(0.f, std::min(screenPts[i].x, boundBoxMin.x));
        boundBoxMin.y = std::max(0.f, std::min(screenPts[i].y, boundBoxMin.y));

        boundBoxMax.x = std::min(m_WindowWidth - 1.f, std::max(screenPts[i].x, boundBoxMax.x));
        boundBoxMax.y = std::min(m_WindowHeight - 1.f, std::max(screenPts[i].y, boundBoxMax.y));
    }

#pragma omp parallel for
    for (int y = (int)boundBoxMin.y; y <= (int)boundBoxMax.y; ++y) {
        for (int x = (int)boundBoxMin.x; x <= (int)boundBoxMax.x; ++x) {
            vec3 screenBar = Barycentric(screenPts, vec2(x, y));    // 屏幕空间重心坐标
            if (screenBar.x < 0.f || screenBar.y < 0.f || screenBar.z < 0.f) {
                continue;
            }
            // 计算实际空间的重心坐标 透视矫正 1/zt = a*1/z1 + b*1/z2 + c*1/z3  It/zt = a*I1/z1 + b*I2/z2 + c*I3/z3 然后view->proj后w分量是z值
            vec3 clipBar = vec3(screenBar.x / clipPts[0].w, screenBar.y / clipPts[1].w, screenBar.z / clipPts[2].w);
            clipBar = clipBar / (clipBar.x + clipBar.y + clipBar.z);
            float depth = clipBar.x * clipPts[0].z / clipPts[0].w  + clipBar.y * clipPts[1].z / clipPts[1].w + clipBar.z * clipPts[2].z / clipPts[2].w;
            // 深度测试 z从里到外增大 [far, near]->[0, 1]
            if (depth < m_Zbuffer[x + y * m_WindowWidth]) {
                continue;
            }
            QRgb color;
            bool discard = shader->Fragment(clipBar, color);
            if (!discard) {
                m_PixelBuffer[x + y * m_WindowWidth] = color;
                m_Zbuffer[x + y * m_WindowWidth] = depth;
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


