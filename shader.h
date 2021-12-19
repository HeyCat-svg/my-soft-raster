#ifndef SHADER_H
#define SHADER_H

#include "geometry.h"
#include "tgaimage.h"
#include "model.h"
#include <QRgb>

///////////////////////////////////////// SHADER ENV ////////////////////////////

extern mat4x4 MODEL_MATRIX;                         // model to world
extern mat4x4 MODEL_INVERSE_TRANSPOSE_MATRIX;       // model的逆转置矩阵
extern mat4x4 VIEW_MATRIX;                          // world to view
extern mat4x4 PROJ_MATRIX;                          // view to clip space
extern mat4x4 VP_MATRIX;                            // proj * view
extern vec4 LIGHT0;                                 // 向量或位置 区别在于w分量1or0
extern vec3 CAMERA_POS;

void SetModelMatrix(mat4x4& mat);
void SetViewMatrix(mat4x4& mat);
void SetViewMatrix(vec3& cameraPos, mat4x4& lookAtMat);
void SetProjectionMatrix(mat4x4& mat);
void SetCameraAndLight(vec3& cameraPos, vec4& light);
vec3 NormalObjectToWorld(const vec3& n);
vec3 Reflect(const vec3& inLightDir, const vec3& normal);
float clamp01(float v);

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
    Model* model;
    vec3 lightColor;
    float specStrength;

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
    virtual ~GeneralShader() {
        delete diffuseTexture;
        delete model;
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
        float spec = std::pow(clamp01(halfDir * worldNormal), 16);
        vec3 col = clamp01(ambient + diff + spec) * mul(lightColor, proj<3>(albedo));
        col = col * 255.f;
        outColor = (255 << 24) | ((uint8_t)col[0] << 16) | ((uint8_t)col[1] << 8) | ((uint8_t)col[2]);
        return false;
    }

    void SetResource(Model* _model, TGAImage* _diffuseTexture, TGAImage* _normalTexture, vec3 _lightColor = {1, 1, 1}, float _specStrength = 1.f) {
        diffuseTexture = _diffuseTexture;
        normalTexture = _normalTexture;
        model = _model;
        lightColor = _lightColor;
        specStrength = _specStrength;
    }
};

#endif // SHADER_H
