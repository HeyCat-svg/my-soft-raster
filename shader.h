#ifndef SHADER_H
#define SHADER_H

#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include "accel.h"
#include <QRgb>
#include <QImage>
#include <cstdlib>

///////////////////////////////////////// SHADER ENV ////////////////////////////

struct ShaderLight {
    vec4 lightPos;
    vec3 lightColor;
    float intensity;
};

extern mat4x4 MODEL_MATRIX;                         // model to world
extern mat4x4 MODEL_INVERSE_TRANSPOSE_MATRIX;       // model的逆转置矩阵
extern mat4x4 VIEW_MATRIX;                          // world to view
extern mat4x4 V_TRANSPOSE_MATRIX;
extern mat4x4 V_INVERSE_MATRIX;
extern mat4x4 MV_INVERSE_TRANSPOSE_MATRIX;
extern mat4x4 PROJ_MATRIX;                          // view to clip space
extern mat4x4 VP_MATRIX;                            // proj * view
extern vec4 LIGHT0;                                 // 向量或位置 区别在于w分量1or0
extern std::vector<ShaderLight> LIGHTS;
extern vec3 CAMERA_POS;
extern vec4 _ProjectionParams;                       // x=1.0(或-1.0 表示y反转了) y=1/near z=1/far w=(1/far-1/near)

void SetModelMatrix(mat4x4& mat);
void SetViewMatrix(const mat4x4& mat);
void SetViewMatrix(const vec3& cameraPos, const mat4x4& lookAtMat);
void SetProjectionMatrix(const mat4x4& mat);
void SetCameraAndLight(vec3 cameraPos, vec4 light);
void SetLightArray(const ShaderLight* lights, int n);
vec3 NormalObjectToWorld(const vec3& n);
vec3 NormalObjectToView(const vec3& n);
vec3 CoordNDCToView(const vec3& p);
vec3 CoordNDCToView(vec3 p, int);           // int 用于区分参数是否引用
vec3 GetNDC(vec2 ndcXY, float* zbuffer, int zbufferWidth, int zbufferHeight);
vec3 Reflect(const vec3& inLightDir, const vec3& normal);
vec3 Refract(const vec3& inLightDir, vec3 normal, float refractiveIndex);
float clamp01(float v);
template<int n> vec<n> clamp01(vec<n> v) {
    vec<n> ret;
    for (int i = n; i--; ret[i] = (v[i] > 1.f) ? 1.f : ((v[i] < 0.f) ? 0.f : v[i]));
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////

class IShader {
public:
    virtual ~IShader() {};
    virtual vec4 Vertex(int iface, int nthvert) = 0;
    virtual void Geometry() {};     // 几何着色器可以拿到完整的图元和图元所有的顶点 修改顶点数据（目前功能）或增添顶点（暂且没做）
    virtual bool Fragment(vec3 barycentric, QRgb& outColor) = 0;
};

class GeneralShader : public IShader {
    TGAImage* diffuseTexture;
    TGAImage* normalTexture;
    TGAImage* specTexture;
    QImage* shadowMap;
    QImage* AOMap;      // 环境光遮蔽贴图
    Model* model;
    vec3 lightColor;
    float specStrength;
    mat4x4 world2Light;
    float shadowBias;

    struct v2f {
        vec3 normal;
        vec2 uv;
        vec3 worldPos;
        // 不能在vertex阶段构造转换矩阵 因为经过插值后 矩阵的模可能!=1 因为矩阵中的向量被插值后模长小于1
        // 但为了减少计算量 对矩阵插值影响轻微 这里默许这么做
        mat3x3 tanToWorld;
        vec4 clipPos;
    };

    v2f vertOutput[3];

 public:
    GeneralShader(Model* _model, TGAImage* _diffuseTexture, TGAImage* _normalTexture, TGAImage* _specTexture, QImage* _shadowMap, const mat4x4& _world2Light, QImage* _AOMap, vec3 _lightColor = {1, 1, 1}, float _specStrength = 16.f, float _shadowBias = 0.02f) :
        model(_model), diffuseTexture(_diffuseTexture), normalTexture(_normalTexture), specTexture(_specTexture),
        shadowMap(_shadowMap), world2Light(_world2Light), AOMap(_AOMap), lightColor(_lightColor), specStrength(_specStrength),
        shadowBias(_shadowBias)
    {}

    virtual ~GeneralShader() {
        delete diffuseTexture;
        delete normalTexture;
        delete specTexture;
    };

    virtual vec4 Vertex(int iface, int nthvert) override {
        v2f o;
        o.normal = NormalObjectToWorld(model->normal(iface, nthvert)).normalize();
        o.uv = model->uv(iface, nthvert);
        vec4 worldPos = MODEL_MATRIX * embed<4>(model->vert(iface, nthvert));
        o.worldPos = proj<3>(worldPos);
        o.clipPos = VP_MATRIX * worldPos;
        vertOutput[nthvert] = o;

        return o.clipPos;
    }

    virtual void Geometry() override {
        // 切线计算 http://blog.sina.com.cn/s/blog_15ff6002b0102y8b9.html
        vec2 uvs[3];
        for (int i = 0; i < 3; ++i) {
            uvs[i] = vertOutput[i].uv;
        }
        float prefix = 1.f / ((uvs[1].x - uvs[0].x) * (uvs[2].y - uvs[0].y) - (uvs[1].y - uvs[0].y) * (uvs[2].x - uvs[0].x));
        vec3 q0 = vertOutput[1].worldPos - vertOutput[0].worldPos;
        vec3 q1 = vertOutput[2].worldPos - vertOutput[0].worldPos;
        vec3 tangent = prefix * ((uvs[2].y - uvs[0].y) * q0 + (uvs[0].y - uvs[1].y) * q1);

        // 分别计算每个顶点的实际切线
        for (int i = 0; i < 3; ++i) {
            vec3 verTan = (tangent - (tangent * vertOutput[i].normal) * vertOutput[i].normal).normalize();
            vec3 verBitan = cross(vertOutput[i].normal, verTan);
            vertOutput[i].tanToWorld.set_col(0, verTan);
            vertOutput[i].tanToWorld.set_col(1, verBitan);
            vertOutput[i].tanToWorld.set_col(2, vertOutput[i].normal);

            static bool flag = false;
            if (!flag) {
                vec3 bitangent = prefix * ((uvs[0].x - uvs[2].x) * q0 + (uvs[1].x - uvs[0].x) * q1);
                qDebug() << verBitan * bitangent;
                flag = true;
            }
        }
    }

    virtual bool Fragment(vec3 barycentric, QRgb& outColor) override {
        static int diffuseWidth = diffuseTexture->get_width();
        static int diffuseHeight = diffuseTexture->get_height();
        static int normalWidth = normalTexture->get_width();
        static int normalHeight = normalTexture->get_height();
        static int specWidth = specTexture->get_width();
        static int specHeight = specTexture->get_height();
        static int shadowMapWidth = shadowMap->width();
        static int shadowMapHeight = shadowMap->height();
        static int AOMapWidth = AOMap->width();
        static int AOMapHeight = AOMap->height();

        vec2 uv = {0, 0};
        vec3 worldPos = {0, 0, 0};
        vec4 clipPos = {0, 0, 0, 0};
        mat3x3 tanToWorld = mat3x3::zero();
        for (int i = 0; i < 3; ++i) {
            uv = uv + barycentric[i] * vertOutput[i].uv;
            worldPos = worldPos + barycentric[i] * vertOutput[i].worldPos;
            tanToWorld = tanToWorld + vertOutput[i].tanToWorld * barycentric[i];
            clipPos = clipPos + vertOutput[i].clipPos * barycentric[i];
        }
        // 计算由法线贴图获取的切线空间法向量 再转换为世界空间
        TGAColor rawTanNormal = normalTexture->get(uv.x * normalWidth, uv.y * normalHeight);
        vec3 tanNormal = {rawTanNormal[2] / 255.f * 2.f - 1.f, rawTanNormal[1] / 255.f * 2.f - 1.f, rawTanNormal[0] / 255.f * 2.f - 1.f};
        vec3 worldNormal = (tanToWorld * tanNormal).normalize();
        vec2 screenPos = proj<2>(clipPos / clipPos.w);
        screenPos = 0.5f * screenPos + 0.5f;

        vec3 lightDir = (LIGHT0.w == 0) ? embed<3>(LIGHT0) : (embed<3>(LIGHT0) - worldPos).normalize();
        vec3 viewDir = (CAMERA_POS - worldPos).normalize();
        vec3 halfDir = (lightDir + viewDir).normalize();
        // 图片加载已经经过y反转 不需要reverse y
        TGAColor rawAlbedo = diffuseTexture->get(uv.x * diffuseWidth, uv.y * diffuseHeight);
        vec4 albedo = {rawAlbedo[2] / 255.f, rawAlbedo[1] / 255.f, rawAlbedo[0] / 255.f, rawAlbedo[3] / 255.f};
        float ambient = 0.3f * ((AOMap->pixel(screenPos.x * AOMapWidth, screenPos.y * AOMapHeight) & 0xff) / 255.f);
        float diff = clamp01(worldNormal * lightDir);
        // float specPower = specTexture->get(uv.x * specWidth, uv.y * specHeight)[0] / 255.f;
        float spec = std::pow(clamp01(halfDir * worldNormal), 16);

        // 计算点在light空间的clip坐标 进一步得到shadow值
        vec4 lightP = world2Light * embed<4>(worldPos);
        lightP = lightP / lightP.w;
        // 这里不需要反转shadowMap 因为world2Light中的P_MATRIX已经将y反转过了
        float shadow = (shadowMap->pixel((0.5f * lightP.x + 0.5f) * shadowMapWidth, (0.5f * lightP.y + 0.5f) * shadowMapHeight) & 0xff) / 255.f;
        shadow = 0.3f + 0.7f * ((lightP.z + shadowBias) > shadow);

        vec3 col = clamp01((ambient + shadow * (diff + spec)) * mul(lightColor, proj<3>(albedo)));
        // col = ambient * lightColor;
        col = col * 255.f;
        outColor = (255 << 24) | ((uint8_t)col[0] << 16) | ((uint8_t)col[1] << 8) | ((uint8_t)col[2]);
        return false;
    }
};

class ShadowMapShader : public IShader {
    Model* model;

    struct v2f {
        // 不使用z/w 然后进行插值的原因在于z/w在世界空间中的变化是非线性的 因此要分别对z和w线性插值后 再做透视除法算出深度
        vec4 clipPos;
    };

    v2f vertOutput[3];

public:
    ShadowMapShader(Model* _model) : model(_model) {}

    virtual vec4 Vertex(int iface, int nthvert) override {
        vertOutput[nthvert].clipPos = VP_MATRIX * MODEL_MATRIX * embed<4>(model->vert(iface, nthvert));
        return vertOutput[nthvert].clipPos;
    }

    virtual bool Fragment(vec3 barycentric, QRgb& outColor) override {
        vec4 clipPos = barycentric.x * vertOutput[0].clipPos + barycentric.y * vertOutput[1].clipPos + barycentric.z * vertOutput[2].clipPos;
        uint8_t depthColor = (clipPos.z / clipPos.w) * 255;
        outColor = (255 << 24) | (depthColor << 16) | (depthColor << 8) | depthColor;
        return false;
    }
};

/* horizon-based AO 或许可以在这里计算AO的同时计算光照颜色 */
class HBAOShader : public IShader {
    Model* model;
    float* zbuffer;                 // 正宗的zbuffer 里面是经过插值的ndc空间z值 [far, near]->[0, 1]
    int zbufferWidth, zbufferHeight;
    float sampleRadius, sampleCount, dirCount;        // 采样半径(uv) 采样数目 方向数目

    struct v2f {
        vec3 viewNormal;
        vec4 clipPos;               // 用于获取屏幕坐标的 和 手动zclip用的
    };

    v2f vertOutput[3];

    inline float WeightFun(float w) {
        w = clamp01(w);
        return 1.f - w * w;
    }

    // 对某一方向上的occlusion进行积分
    float CalAmbientOcclusionIntegralInOneDir(const vec3& originNDC, const vec2& dir, const vec3& normal) {
        float totalOcclusion = 0.f;
        float topSin = 0.03f;     // 记录积分开始的片段 从非零开始意味着忽略sinθ小于0.03的遮挡

        vec3 originView = CoordNDCToView(originNDC);
        vec2 texelSizeStep = mul(dir, vec2(2.f / zbufferWidth, 2.f / zbufferHeight));
        vec3 tangent = CoordNDCToView(GetNDC(proj<2>(originNDC) + texelSizeStep, zbuffer, zbufferWidth, zbufferHeight), 0) - originView;
        tangent = (tangent - (normal * tangent) * normal).normalize();
        vec2 stepNDC = (sampleRadius / sampleCount) * dir;

        // begin march
        vec3 curSampleNDC = GetNDC(proj<2>(originNDC) + stepNDC, zbuffer, zbufferWidth, zbufferHeight);
        for (int i = 0; i < sampleCount; ++i) {
            // 校验NDC是否在合法范围内
            if (curSampleNDC.x < -1 || curSampleNDC.x > 1 || curSampleNDC.y < -1 || curSampleNDC.y > 1) {
                break;
            }
            // 排除采样到非物体的zbuffer
            if (curSampleNDC.z < 0) {
                continue;
            }

            vec3 sampleView = CoordNDCToView(curSampleNDC);
            vec3 horizonVec = sampleView - originView;
            float horizonVecLength = horizonVec.norm();

            // 这个方向的ambient被完全遮住
            if (tangent * horizonVec < 0) {
                return 1.f;
            }
            float curSin = normal * horizonVec / horizonVecLength; // sinθ
            float diff = std::max(curSin - topSin, 0.f);
            topSin = std::max(topSin, curSin);

            totalOcclusion += diff * WeightFun((float)i / sampleCount);

            curSampleNDC = GetNDC(proj<2>(curSampleNDC) + stepNDC, zbuffer, zbufferWidth, zbufferHeight);
        }

        return totalOcclusion;
    }

 public:
    HBAOShader(Model* _model, float* _zbuffer, int _zbufferWidth, int _zbufferHeight, float _sampleRadius = 0.2f, float _sampleCount = 5.f, float _dirCount = 6.f) :
        model(_model), zbuffer(_zbuffer), zbufferWidth(_zbufferWidth), zbufferHeight(_zbufferHeight),
        sampleRadius(_sampleRadius), sampleCount(_sampleCount), dirCount(_dirCount)
    {}

    virtual vec4 Vertex(int iface, int nthvert) override {
        v2f o;
        o.viewNormal = NormalObjectToView(model->normal(iface, nthvert)).normalize();
        o.clipPos = VP_MATRIX * MODEL_MATRIX * embed<4>(model->vert(iface, nthvert));
        vertOutput[nthvert] = o;
        return o.clipPos;
    }

    virtual bool Fragment(vec3 barycentric, QRgb& outColor) override {
        vec3 normal(0, 0, 0);
        vec4 clipPos(0, 0, 0, 0);
        for (int i = 0; i < 3; ++i) {
            normal = normal + barycentric[i] * vertOutput[i].viewNormal;
            clipPos = clipPos + barycentric[i] * vertOutput[i].clipPos;
        }
        normal.normalize();
        vec3 ndc = proj<3>(clipPos / clipPos.w);
        vec2 screenPos(0.5f * ndc.x + 0.5f, 0.5f * ndc.y + 0.5f);
        float depth = zbuffer[(int)(screenPos.y * zbufferHeight) * zbufferWidth + (int)(screenPos.x * zbufferWidth)];
        // 手动裁剪
        if (ndc.z < (depth - 1e-2)) {
            return true;
        }

        // 开始旋转采样
        float totalAO = 0.f;        // 环境光被阻挡的部分
        float rotationStep = 2 * PI / dirCount;
        float angle = ((float)std::rand() / RAND_MAX) * rotationStep;
        for (int i = 0; i < dirCount; ++i, angle += rotationStep) {
            vec2 dir(std::cos(angle), std::sin(angle));
            // 乘上rotationStep是为了计算球面积分的dθ部分 对上半部经度积分后 再对纬度积分
            totalAO += rotationStep * CalAmbientOcclusionIntegralInOneDir(ndc, dir, normal);
        }

        float ambient = 1.f - 1.f / (2.f * PI) * totalAO;   // 1/2pi 系数是归一化半球面积分 半球面积为1/2pi
        outColor = (255 << 24) | ((uint8_t)(ambient * 255) << 16) | ((uint8_t)(ambient * 255) << 8) | (uint8_t)(ambient * 255);

        return false;
    }
};

class ZWriteShader : public IShader {
    Model* model;

public:
    ZWriteShader(Model* _model) : model(_model) {}

    virtual vec4 Vertex(int iface, int nthvert) override {
        return VP_MATRIX * MODEL_MATRIX * embed<4>(model->vert(iface, nthvert));
    }

    virtual bool Fragment(vec3 barycentric, QRgb& outColor) override {
        outColor = (255 << 24);     // black
        return false;
    }
};

class RayTracerShader : public IShader {
    const int MAX_DEPTH = 4;        // 光线追踪的最深递归深度 最多计算MAX_DEPTH次反射

    vec3 screenMesh[2][3] = {
        {{-1.f, 1.f, 1.f}, {-1.f, -1.f, 1.f}, {1.f, -1.f, 1.f}},
        {{-1.f, 1.f, 1.f}, {1.f, -1.f, 1.f}, {1.f, 1.f, 1.f}}
    };
    float fov, aspect, halfWidth, halfHeight;
    Accel* modelAccel;      // 模型三角面片搜索加速结构
    Model* model;

    struct v2f {
        vec3 rayDir;
    };

    v2f vertOutput[3];

    vec3 CastRay(const Ray& ray, int depth = 0) {
        HitResult hitResult;

        if (depth > MAX_DEPTH || !modelAccel->Intersect(ray, hitResult)) {
            return vec3(0, 0, 0);   // 将来要替换成天空盒
        }
        int faceIdx = hitResult.hitIdx;
        vec3 bar = hitResult.barycentric;
        vec3 normal = {0, 0, 0};
        vec2 uv = {0, 0};
        vec3 worldPos = {0, 0, 0};
        for (int i = 0; i < 3; ++i) {
            normal = normal + bar[i] * model->normal(faceIdx, i);
            uv = uv + bar[i] * model->uv(faceIdx, i);
            worldPos = worldPos + bar[i] * model->vert(faceIdx, i);
        }
        normal.normalize();

        // 计算反射光线
        vec3 reflectDir = Reflect(ray.dir, normal).normalize();
        vec3 reflectOri = (reflectDir * normal < 0) ? worldPos - normal * 1e-3 : worldPos + normal * 1e-3;
        Ray reflectRay(reflectOri, reflectDir);
        vec3 reflectColor = CastRay(reflectRay, depth + 1);

        // 计算折射光线
        vec3 refractColor;
        vec3 refractDir = Refract(ray.dir, normal, 1.333f);     // 折射率1.333 玻璃
        if (refractDir.x > 1) {
            refractColor = vec3(0, 0, 0);
        }
        else {
            vec3 refractOri = (refractDir * normal < 0) ? worldPos - normal * 1e-3 : worldPos + normal * 1e-3;
            Ray refractRay(refractOri, refractDir);
            refractColor = CastRay(refractRay, depth + 1);
        }

        // 开始计算当前光线碰撞点的颜色
        float diff = 0, spec = 0;
        int lightNum = LIGHTS.size();
        for (int i = 0; i < lightNum; ++i) {
            const ShaderLight& light = LIGHTS[i];
            vec3 lightDir = (light.lightPos.w == 0) ? embed<3>(light.lightPos) : (embed<3>(light.lightPos) - worldPos).normalize();
            vec3 viewDir = (-ray.dir).normalize();
            vec3 halfDir = (lightDir + viewDir).normalize();
            Ray lightRay(worldPos + lightDir * 1e-3, lightDir);
            HitResult trashResult;
            if (modelAccel->Intersect(lightRay, trashResult, true)) {
                continue;
            }
            diff += clamp01(lightDir * normal) * light.intensity;
            spec += std::pow(clamp01(halfDir * normal), 125) * light.intensity;
        }

        return vec3(0.6, 0.7, 0.8) * diff * 0.0f + vec3(1, 1, 1) * spec * 0.5f + reflectColor * 0.1f + refractColor * 0.8f;
    }

public:
    RayTracerShader(Model* _model, Accel* _modelAccel, float _fov=PI/3, float _aspect=1.f) :
        model(_model), modelAccel(_modelAccel), fov(_fov), aspect(_aspect)
    {
        halfWidth = aspect * std::tan(fov / 2.f);
        halfHeight = std::tan(fov / 2.f);
    }

    // 只是渲染长方形画面的两个三角形 中间的像素靠光栅化插值
    virtual vec4 Vertex(int iface, int nthvert) override {
        v2f o;
        const vec3& meshP = screenMesh[iface][nthvert];
        o.rayDir = vec3(meshP.x * halfWidth, meshP.y * halfHeight, -1.f);
        // view的逆矩阵的逆转置矩阵就是view的转置->xxx 逆转置用于转换法向量的 将向量从view到world
        o.rayDir = proj<3>(V_INVERSE_MATRIX * embed<4>(o.rayDir, 0)).normalize();
        vertOutput[nthvert] = o;
        return vec4(meshP.x, -meshP.y, meshP.z, 1.f);
    }

    virtual bool Fragment(vec3 barycentric, QRgb& outColor) override {
        vec3 rayDir = {0, 0, 0};
        for (int i = 0; i < 3; ++i) {
            rayDir = rayDir + barycentric[i] * vertOutput[i].rayDir;
        }
        rayDir.normalize();
        Ray ray(CAMERA_POS, rayDir);

        vec3 col = clamp01(CastRay(ray));

        col = col * 255.f;

        outColor = (255 << 24) | ((uint8_t)col[0] << 16) | ((uint8_t)col[1] << 8) | (uint8_t)col[2];
        return false;
    }


};

#endif // SHADER_H
