#include "widget.h"
#include "skybox.h"

// #define SOFT_RASTER
#define RAY_TRACER

Model africanHeadModel("./obj/diablo3_pose/diablo3_pose.obj");

SoftRaster::SoftRaster(QWidget *parent) : QWidget(parent) {
    this->setParent(parent);
    this->setWindowTitle(QString("Soft Raster"));
    this->resize(m_WindowWidth, m_WindowHeight);

    m_AnotherMonitor = new Monitor();
    m_AnotherMonitor->show();

    // init pixel buffer
    m_PixelBuffer = new QRgb[m_WindowWidth * m_WindowHeight];

    // init zbuffer
    m_Zbuffer = new float[m_WindowWidth * m_WindowHeight];
    m_Zbuffer1 = new float[m_WindowWidth * m_WindowHeight];

    // init shadow map
    m_ShadowMap = new QRgb[m_WindowWidth * m_WindowHeight];

    // init AO map
    m_AOMap = new QRgb[m_WindowWidth * m_WindowHeight];

    // set shader env
    // 设置模型TRS
    vec3 translate(0, 0, 0);
    vec3 rotation(0, 0, 0);
    vec3 scale(1.2, 1.2, 1.2);
    mat4x4 model = TRS(translate, rotation, scale);
    SetModelMatrix(model);

    // 设置相机参数
    vec3 worldUp(0, 1, 0);
    vec3 cameraPos(0.5f, 0.5f, 2.f);       // 0.5 0.5 2.5
    vec3 lookDir = vec3(0, 0, 0) - cameraPos;
    m_Camera = new Camera(cameraPos, lookDir, PI / 3.f, 1.f, 0.3f, 10.f);

    // 设置光源
    vec3 lightColor(1, 1, 1);
    vec3 lightPos(1, 1, 1);
    vec3 lightDir = vec3(0, 0, 0) - lightPos;
    m_PointLight = new Light(lightColor, lightPos, lightDir, ProjectionType::PERSP);
    SetCameraAndLight(cameraPos, embed<4>(lightPos));
    SetViewMatrix(m_PointLight->GetViewMatrix());       // 利用set函数 将raw view和proj矩阵转变成最后使用的VP_MATRIX
    SetProjectionMatrix(m_PointLight->GetProjectionMatrix());
    m_PointLight->SetWorld2Light(VP_MATRIX);

    // 设置光源数组
    ShaderLight lights[2];
    lights[0] = {{-1.f, 1.f, 1.f, 1.f}, {1, 1, 1}, 1.2f};
    lights[1] = {{1.f, 1.f, 1.f, 1.f}, {1, 1, 1}, 1.2f};
    SetLightArray(lights, 2);

    // 初始化模型加速结构
    m_ModelAccel = new Accel(&africanHeadModel);
    m_ModelAccel->Build();
    qDebug() << "face number: " << africanHeadModel.nfaces();
    Ray ray = {{0, 0, 1}, {0, 0, -1}};
    HitResult hitResult;
    m_ModelAccel->Intersect(ray, hitResult);
    qDebug() << hitResult.t << '\t' << hitResult.hitIdx << '\t' << hitResult.barycentric.x << ' ' <<
                hitResult.barycentric.y << ' ' << hitResult.barycentric.z << '\t' <<
                hitResult.hitPoint.x << ' ' << hitResult.hitPoint.y << ' ' << hitResult.hitPoint.z;

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
    Skybox* skybox = new Skybox("./resources/skybox/skybox");
    m_Shader = new GeneralShader(
                &africanHeadModel, diffuseImg, normalImg, specImg,
                new QImage((uchar*)m_ShadowMap, m_WindowWidth, m_WindowHeight, QImage::Format_ARGB32),
                m_PointLight->GetWorld2Light(),
                new QImage((uchar*)m_AOMap, m_WindowWidth, m_WindowHeight, QImage::Format_ARGB32)
                );
    m_ShadowMapShader = new ShadowMapShader(&africanHeadModel);
    m_HBAOShader = new HBAOShader(&africanHeadModel, m_Zbuffer1, m_WindowWidth, m_WindowHeight);
    m_ZWriteShader = new ZWriteShader(&africanHeadModel);
    m_RayTracerShader = new RayTracerShader(&africanHeadModel, m_ModelAccel, skybox);

    // start repaint timer
    m_RepaintTimer = startTimer(m_RepaintInterval);
}


SoftRaster::~SoftRaster() {
    killTimer(m_RepaintTimer);
    delete m_AnotherMonitor;
    delete[] m_PixelBuffer;
    delete[] m_Zbuffer;
    delete[] m_Zbuffer1;
    delete[] m_ShadowMap;
    delete[] m_AOMap;
    delete m_Shader;
    delete m_ShadowMapShader;
    delete m_HBAOShader;
    delete m_ZWriteShader;
    delete m_RayTracerShader;
    delete m_ModelAccel;
    delete m_PointLight;
    delete m_Camera;
}


void SoftRaster::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    QImage image((uchar*)m_PixelBuffer, m_WindowWidth, m_WindowHeight, QImage::Format_ARGB32);

    // timer start
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
            m_Zbuffer1[i * m_WindowWidth + j] = Z_MIN;
            m_ShadowMap[i * m_WindowWidth + j] = bgColor;
            m_AOMap[i * m_WindowWidth + j] = bgColor;
        }
    }

///////////////////////////////////////// SOFT RASTER START ////////////////////////////
#ifdef SOFT_RASTER
    /// HBAO rendering
    // Pass 0: z write
    {
        SetViewMatrix(m_Camera->GetViewMatrix());
        SetProjectionMatrix(m_Camera->GetProjectionMatrix());
        int faceCount = africanHeadModel.nfaces();
        for (int i = 0; i < faceCount; ++i) {
            vec4 clipPts[3];
            for (int j = 0; j < 3; ++j) {
                clipPts[j] = m_ZWriteShader->Vertex(i, j);
            }
            // 将深度写入m_Zbuffer1
            Triangle(clipPts, m_ZWriteShader, m_PixelBuffer, m_Zbuffer1);
        }
    }

    // Pass 1: draw HBAO
    {
        int faceCount = africanHeadModel.nfaces();
        for (int i = 0; i < faceCount; ++i) {
            vec4 clipPts[3];
            for (int j = 0; j < 3; ++j) {
                clipPts[j] = m_HBAOShader->Vertex(i, j);
            }
            // 将深度写入m_Zbuffer1
            Triangle(clipPts, m_HBAOShader, m_AOMap, m_Zbuffer);
        }
    }

    /// shadow rendering
    // Pass 2: draw shadow map
    {
#pragma omp parallel for
        for (int i = 0; i < m_WindowHeight; ++i) {
            for (int j = 0; j < m_WindowWidth; ++j) {
                m_Zbuffer[i * m_WindowWidth + j] = Z_MIN;
            }
        }
        SetViewMatrix(m_PointLight->GetViewMatrix());
        SetProjectionMatrix(m_PointLight->GetProjectionMatrix());
        int faceCount = africanHeadModel.nfaces();
        for (int i = 0; i < faceCount; ++i) {
            vec4 clipPts[3];
            for (int j = 0; j < 3; ++j) {
                clipPts[j] = m_ShadowMapShader->Vertex(i, j);
            }
            Triangle(clipPts, m_ShadowMapShader, m_ShadowMap, m_Zbuffer);
        }
    }

    /// blin phong rendering
    // Pass 3: draw model
    {
#pragma omp parallel for
        for (int i = 0; i < m_WindowHeight; ++i) {
            for (int j = 0; j < m_WindowWidth; ++j) {
                m_Zbuffer[i * m_WindowWidth + j] = Z_MIN;
                m_PixelBuffer[i * m_WindowWidth + j] = bgColor;
            }
        }
        SetViewMatrix(m_Camera->GetViewMatrix());
        SetProjectionMatrix(m_Camera->GetProjectionMatrix());
        int faceCount = africanHeadModel.nfaces();
        for (int i = 0; i < faceCount; ++i) {
            vec4 clipPts[3];
            for (int j = 0; j < 3; ++j) {
                clipPts[j] = m_Shader->Vertex(i, j);
            }
            m_Shader->Geometry();
            Triangle(clipPts, m_Shader, m_PixelBuffer, m_Zbuffer);
        }
    }
#endif
///////////////////////////////// SOFT RASTER END /////////////////////////////

///////////////////////////////// RAY TRACER START ////////////////////////////
#ifdef RAY_TRACER
    SetViewMatrix(m_Camera->GetViewMatrix());
    SetProjectionMatrix(m_Camera->GetProjectionMatrix());
    // 光栅化的步骤是为了插值ray 实际只有两个三角面片构成的长方形mesh
    for (int i = 0; i < 2; ++i) {
        vec4 clipPts[3];
        for (int j = 0; j < 3; ++j) {
            clipPts[j] = m_RayTracerShader->Vertex(i, j);
        }
        Triangle(clipPts, m_RayTracerShader, m_PixelBuffer, m_Zbuffer);
    }
#endif
///////////////////////////////// RAY TRACER END ////////////////////////////

    // timer end
    QueryPerformanceCounter(&endTime);
    runtime = (((endTime.QuadPart - startTime.QuadPart) * 1000.0f) / cpuFreq.QuadPart);
    qDebug() << "runtime: " << runtime << "ms";


    // draw image on window
    painter.drawImage(0, 0, image);
    // m_AnotherMonitor->Draw(m_ShadowMap, m_WindowWidth, m_WindowHeight);
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


void SoftRaster::Triangle(vec4 *clipPts, IShader* shader, QRgb* renderTarget, float* zbuffer) {
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
            float depth = (clipBar.x * clipPts[0].z + clipBar.y * clipPts[1].z + clipBar.z * clipPts[2].z) / (clipBar.x * clipPts[0].w + clipBar.y * clipPts[1].w + clipBar.z * clipPts[2].w);
            // 深度测试 z从里到外增大 [far, near]->[0, 1]
            if (depth < zbuffer[x + y * m_WindowWidth]) {
                continue;
            }
            QRgb color;
            bool discard = shader->Fragment(clipBar, color);
            if (!discard) {
                renderTarget[x + y * m_WindowWidth] = color;
                zbuffer[x + y * m_WindowWidth] = depth;
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


