#include "shader.h"

mat4x4 MODEL_MATRIX;
mat4x4 MODEL_INVERSE_TRANSPOSE_MATRIX;      // model的逆转置矩阵
mat4x4 VIEW_MATRIX;
mat4x4 V_TRANSPOSE_MATRIX;
mat4x4 V_INVERSE_MATRIX;
mat4x4 MV_INVERSE_TRANSPOSE_MATRIX;         // (view * model)的逆转置矩阵
mat4x4 PROJ_MATRIX;
mat4x4 VP_MATRIX;
vec4 LIGHT0;
std::vector<ShaderLight> LIGHTS;                   // 光源信息数组
vec3 CAMERA_POS;
vec4 _ProjectionParams;                      // x=1.0(或-1.0 表示y反转了) y=1/near z=1/far w=(1/far-1/near)

void SetModelMatrix(mat4x4& mat) {
    MODEL_MATRIX = mat;
    MODEL_INVERSE_TRANSPOSE_MATRIX = mat.invert_transpose();
}

void SetViewMatrix(const mat4x4& mat) {
    // 从世界坐标的左手系变为view空间的右手系需要将z反转
    VIEW_MATRIX = mat;
    VIEW_MATRIX[2][0] = -VIEW_MATRIX[2][0];
    VIEW_MATRIX[2][1] = -VIEW_MATRIX[2][1];
    VIEW_MATRIX[2][2] = -VIEW_MATRIX[2][2];
    VIEW_MATRIX[2][3] = -VIEW_MATRIX[2][3];

    MV_INVERSE_TRANSPOSE_MATRIX = (VIEW_MATRIX * MODEL_MATRIX).invert_transpose();
    V_TRANSPOSE_MATRIX = VIEW_MATRIX.transpose();
    V_INVERSE_MATRIX = VIEW_MATRIX.invert();
}

void SetViewMatrix(const vec3& cameraPos, const mat4x4& lookAtMat) {
    mat4x4 translateMat = mat4x4::identity();
    translateMat[0][3] = -cameraPos.x;
    translateMat[1][3] = -cameraPos.y;
    translateMat[2][3] = -cameraPos.z;

    // 先将相机坐标系平移到原点 然后旋转
    mat4x4 view = lookAtMat * translateMat;
    SetViewMatrix(view);
}

void SetProjectionMatrix(const mat4x4& mat) {
    static mat4x4 projPrefix = mat4x4::identity();
    static bool isPrefixInit = false;

    // opengl范式的投影矩阵[znear, zfar]->[-1, 1] 更改映射[znear, zfar]->[1, 0]
    if (!isPrefixInit) {
        projPrefix[1][1] = -1.f;    // reverse Y 因为屏幕坐标是以左上角为原点
        projPrefix[2][2] = -0.5f;
        projPrefix[2][3] = 0.5f;
        isPrefixInit = true;
    }

    // 计算并更新projection params
    _ProjectionParams.x = -1.f;
    _ProjectionParams.y = (mat[2][2] - 1.f) / mat[2][3];                // 1/near
    _ProjectionParams.z = (mat[2][2] + 1.f) / mat[2][3];                // 1/far
    _ProjectionParams.w = _ProjectionParams.z - _ProjectionParams.y;    // 1/far-1/near

    PROJ_MATRIX = projPrefix * mat;
    VP_MATRIX = PROJ_MATRIX * VIEW_MATRIX;
}

void SetCameraAndLight(vec3 cameraPos, vec4 light) {
    CAMERA_POS = cameraPos;
    LIGHT0 = light;
}

void SetLightArray(const ShaderLight* lights, int n) {
    LIGHTS.clear();
    for (int i = 0; i < n; ++i) {
        LIGHTS.push_back(lights[i]);
    }
}

vec3 NormalObjectToWorld(const vec3& n) {
    return proj<3>(MODEL_INVERSE_TRANSPOSE_MATRIX * embed<4>(n, 0));
}

vec3 NormalObjectToView(const vec3& n) {
    return proj<3>(MV_INVERSE_TRANSPOSE_MATRIX * embed<4>(n, 0));
}

vec3 CoordNDCToView(const vec3& p) {
    vec3 ret;
    ret.z = 1.f / (_ProjectionParams.w * p.z - _ProjectionParams.z);
    ret.x = -p.x * ret.z / PROJ_MATRIX[0][0];
    ret.y = -p.y * ret.z / PROJ_MATRIX[1][1];
    return ret;
}

vec3 CoordNDCToView(vec3 p, int) {
    vec3 ret;
    ret.z = 1.f / (_ProjectionParams.w * p.z - _ProjectionParams.z);
    ret.x = -p.x * ret.z / PROJ_MATRIX[0][0];
    ret.y = -p.y * ret.z / PROJ_MATRIX[1][1];
    return ret;
}

vec3 GetNDC(vec2 ndcXY, float* zbuffer, int zbufferWidth, int zbufferHeight) {
    vec3 ret;
    ret.x = ndcXY.x;
    ret.y = ndcXY.y;
    ret.z = zbuffer[(int)((0.5f * ndcXY.x + 0.5f) * zbufferWidth) + (int)((0.5f * ndcXY.y + 0.5f) * zbufferHeight) * zbufferWidth];
    return ret;
}

/* in  normal
 *   \ |\
 *   _\|
 */
vec3 Reflect(const vec3& inLightDir, const vec3& normal) {
    return 2.f * (-inLightDir * normal) * normal - inLightDir;
}

/* in  normal
 *   \ |\
 *   _\|
 */
vec3 Refract(const vec3& inLightDir, vec3 normal, float refractiveIndex) {
    float cosIn = -inLightDir * normal;
    float nIn, nOut;    // 入射光线和出射光线的折射率
    if (cosIn > 0) {        // 从外部空气射入
        nIn = 1.f;
        nOut = refractiveIndex;
    }
    else {                  // 从模型内部射出
        nIn = refractiveIndex;
        nOut = 1.f;
        cosIn = -cosIn;     // cosIn始终大于0
        normal = -normal;   // normal始终指向入射光线一侧
    }
    float rate = nIn / nOut;    // 折射率之比
    float cosOutSqr = 1.f - rate * rate * (1.f - cosIn * cosIn);

    // 当rate>1时 有一段是全反射而没有折射 因此此时折射角不存在 返回一个标记值
    return (cosOutSqr < 0) ? vec3(2, 0, 0) : (rate * inLightDir + (rate * cosIn - std::sqrt(cosOutSqr)) * normal).normalize();
}

float clamp01(float v) {
    return (v > 1.f) ? 1.f : ((v < 0.f) ? 0.f : v);
}
