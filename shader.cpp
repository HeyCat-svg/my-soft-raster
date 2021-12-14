#include "shader.h"

mat4x4 MODEL_MATRIX;
mat4x4 VIEW_MATRIX;
mat4x4 PROJ_MATRIX;

void SetModelMatrix(mat4x4& mat) {
    MODEL_MATRIX = mat;
}

void SetViewMatrix(mat4x4& mat) {
    // 从世界坐标的左手系变为view空间的右手系需要将z反转
    mat[2][0] = -mat[2][0];
    mat[2][1] = -mat[2][1];
    mat[2][2] = -mat[2][2];
    mat[2][3] = -mat[2][3];

    VIEW_MATRIX = mat;
}

void SetProjectionMatrix(mat4x4& mat) {
    static mat4x4 projPrefix = mat4x4::identity();
    static bool isPrefixInit = false;

    // opengl范式的投影矩阵[znear, zfar]->[-1, 1] 更改映射[znear, zfar]->[1, 0]
    if (!isPrefixInit) {
        projPrefix[2][2] = -0.5f;
        projPrefix[2][3] = 0.5f;
        isPrefixInit = true;
    }

    PROJ_MATRIX = projPrefix * mat;
}
