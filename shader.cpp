#include "shader.h"

mat4x4 MODEL_MATRIX;
mat4x4 MODEL_INVERSE_TRANSPOSE_MATRIX;      // model的逆转置矩阵
mat4x4 VIEW_MATRIX;
mat4x4 PROJ_MATRIX;
mat4x4 VP_MATRIX;
vec4 LIGHT0;
vec3 CAMERA_POS;

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

    PROJ_MATRIX = projPrefix * mat;
    VP_MATRIX = PROJ_MATRIX * VIEW_MATRIX;
}

void SetCameraAndLight(vec3 cameraPos, vec4 light) {
    CAMERA_POS = cameraPos;
    LIGHT0 = light;
}

vec3 NormalObjectToWorld(const vec3& n) {
    vec4 ret = embed<4>(n);
    return proj<3>(MODEL_INVERSE_TRANSPOSE_MATRIX * ret);
}

/* in  normal
 *  |\ |\
 *    \|
 */
vec3 Reflect(const vec3& inLightDir, const vec3& normal) {
    return 2.f * (inLightDir * normal) * normal - inLightDir;
}

float clamp01(float v) {
    return (v > 1.f) ? 1.f : ((v < 0.f) ? 0.f : v);
}
