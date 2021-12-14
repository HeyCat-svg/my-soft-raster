#include "geometry.h"

vec3 cross(const vec3& v1, const vec3& v2) {
    vec3 ret;
    ret.x = v1.y * v2.z - v1.z * v2.y;
    ret.y = v1.z * v2.x - v1.x * v2.z;
    ret.z = v1.x * v2.y - v1.y * v2.x;
    return ret;
}

Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs) {
    Quaternion ret;
    ret.v = lhs.s * rhs.v + rhs.s * lhs.v + cross(lhs.v, rhs.v);
    ret.s = lhs.s * rhs.s - lhs.v * rhs.v;
    return ret;
}

mat4x4 TRS(vec3& translate, vec3& rotation, vec3& scale) {
    mat4x4 scaleMat = mat4x4::identity();
    scaleMat[0][0] = scale[0];
    scaleMat[1][1] = scale[1];
    scaleMat[2][2] = scale[2];

    mat4x4 rotationMat = Quaternion(rotation).ToMatrix();

    mat4x4 translateMat = mat4x4::identity();
    translateMat[0][3] = translate[0];
    translateMat[1][3] = translate[1];
    translateMat[2][3] = translate[2];

    return translateMat * rotationMat * scaleMat;
}

// world左手系->view右手系需要将z反转一下
mat4x4 LookAt(vec3& dir, vec3& up) {
    mat4x4 ret = mat4x4::identity();
    vec3 z = dir.normalize();
    vec3 x = cross(up, z);
    vec3 y = cross(z, x);
    x.normalize();
    y.normalize();
    ret[0][0] = x.x; ret[0][1] = x.y; ret[0][2] = x.z;
    ret[1][0] = y.x; ret[1][1] = y.y; ret[1][2] = y.z;
    ret[2][0] = z.x; ret[2][1] = z.y; ret[2][2] = z.z;

    return ret;
}

mat4x4 Projection(ProjectionType type, float znear, float zfar, float top, float down, float left, float right) {
    mat4x4 ret = mat4x4::identity();

    if (type == ProjectionType::ORTH) {
        ret[0][0] = 2.f / (right - left);
        ret[0][3] = -(right + left) / (right - left);
        ret[1][1] = 2.f / (top - down);
        ret[1][3] = -(top + down) / (top - down);
        ret[2][2] = -2.f / (zfar - znear);
        ret[2][3] = -(zfar + znear) / (zfar - znear);
    }
    else if (type == ProjectionType::PERSP) {
        ret[0][0] = 2.f * znear / (right - left);
        ret[0][2] = (right + left) / (right - left);
        ret[1][1] = 2.f * znear / (top - down);
        ret[1][2] = (top + down) / (top - down);
        ret[2][2] = -(zfar + znear) / (zfar - znear);
        ret[2][3] = -2.f * zfar * znear / (zfar - znear);
        ret[3][2] = -1;
        ret[3][3] = 0;
    }

    return ret;
}

mat4x4 PerspProjection(float fov, float aspect, float znear, float zfar) {
    float halfHeight = std::tan(0.5f * fov) * znear;
    float halfWidth = aspect * halfHeight;
    mat4x4 ret = mat4x4::identity();

    ret[0][0] = znear / halfWidth;
    ret[1][1] = znear / halfHeight;
    ret[2][2] = -(zfar + znear) / (zfar - znear);
    ret[2][3] = -2.f * zfar * znear / (zfar - znear);
    ret[3][2] = -1;
    ret[3][3] = 0;

    return ret;
}
