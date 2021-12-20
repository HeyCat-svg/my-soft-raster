#ifndef SHADER_H
#define SHADER_H

#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include <QRgb>
#include <QImage>

///////////////////////////////////////// SHADER ENV ////////////////////////////

extern mat4x4 MODEL_MATRIX;                         // model to world
extern mat4x4 MODEL_INVERSE_TRANSPOSE_MATRIX;       // model的逆转置矩阵
extern mat4x4 VIEW_MATRIX;                          // world to view
extern mat4x4 PROJ_MATRIX;                          // view to clip space
extern mat4x4 VP_MATRIX;                            // proj * view
extern vec4 LIGHT0;                                 // 向量或位置 区别在于w分量1or0
extern vec3 CAMERA_POS;

void SetModelMatrix(mat4x4& mat);
void SetViewMatrix(const mat4x4& mat);
void SetViewMatrix(const vec3& cameraPos, const mat4x4& lookAtMat);
void SetProjectionMatrix(const mat4x4& mat);
void SetCameraAndLight(vec3 cameraPos, vec4 light);
vec3 NormalObjectToWorld(const vec3& n);
vec3 Reflect(const vec3& inLightDir, const vec3& normal);
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
    };

    v2f vertOutput[3];

 public:
    GeneralShader(Model* _model, TGAImage* _diffuseTexture, TGAImage* _normalTexture, TGAImage* _specTexture, QImage* _shadowMap, const mat4x4& _world2Light, vec3 _lightColor = {1, 1, 1}, float _specStrength = 16.f, float _shadowBias = 0.02f) :
        model(_model), diffuseTexture(_diffuseTexture), normalTexture(_normalTexture), specTexture(_specTexture),
        shadowMap(_shadowMap), world2Light(_world2Light), lightColor(_lightColor), specStrength(_specStrength),
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
        vertOutput[nthvert] = o;

        return VP_MATRIX * worldPos;
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

        vec2 uv = {0, 0};
        vec3 worldPos = {0, 0, 0};
        mat3x3 tanToWorld = mat3x3::zero();
        for (int i = 0; i < 3; ++i) {
            uv = uv + barycentric[i] * vertOutput[i].uv;
            worldPos = worldPos + barycentric[i] * vertOutput[i].worldPos;
            tanToWorld = tanToWorld + vertOutput[i].tanToWorld * barycentric[i];
        }
        // 计算由法线贴图获取的切线空间法向量 再转换为世界空间
        TGAColor rawTanNormal = normalTexture->get(uv.x * normalWidth, uv.y * normalHeight);
        vec3 tanNormal = {rawTanNormal[2] / 255.f * 2.f - 1.f, rawTanNormal[1] / 255.f * 2.f - 1.f, rawTanNormal[0] / 255.f * 2.f - 1.f};
        vec3 worldNormal = (tanToWorld * tanNormal).normalize();

        vec3 lightDir = (LIGHT0.w == 0) ? embed<3>(LIGHT0) : (embed<3>(LIGHT0) - worldPos).normalize();
        vec3 viewDir = (CAMERA_POS - worldPos).normalize();
        vec3 halfDir = (lightDir + viewDir).normalize();
        // 图片加载已经经过y反转 不需要reverse y
        TGAColor rawAlbedo = diffuseTexture->get(uv.x * diffuseWidth, uv.y * diffuseHeight);
        vec4 albedo = {rawAlbedo[2] / 255.f, rawAlbedo[1] / 255.f, rawAlbedo[0] / 255.f, rawAlbedo[3] / 255.f};
        float ambient = 0.2f;
        float diff = clamp01(worldNormal * lightDir);
        // float specPower = specTexture->get(uv.x * specWidth, uv.y * specHeight)[0] / 255.f;
        float spec = std::pow(clamp01(halfDir * worldNormal), 16);

        // 计算点在light空间的clip坐标 进一步得到shadow值
        vec4 lightP = world2Light * embed<4>(worldPos);
        lightP = lightP / lightP.w;
        // 这里不需要反转shadowMap 因为world2Light中的P_MATRIX已经将y反转过了
        float shadow = (shadowMap->pixel((0.5f * lightP.x + 0.5f) * shadowMapWidth, (0.5f * lightP.y + 0.5f) * shadowMapHeight) & 0xff) / 255.f;
        shadow = 0.3f + 0.7f * ((lightP.z + shadowBias) > shadow);

        vec3 col = clamp01((ambient + diff + spec) * shadow * mul(lightColor, proj<3>(albedo)));
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

#endif // SHADER_H
