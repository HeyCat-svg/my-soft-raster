#ifndef CAMERA_H
#define CAMERA_H

#include "geometry.h"


class Camera {
    vec3 m_CameraPos;
    vec3 m_LookDir;
    float m_FOV, m_Aspect, m_Znear, m_Zfar;
    mat4x4 m_ViewMatrix;
    mat4x4 m_ProjectionMatrix;

public:
    Camera(vec3 cameraPos, vec3 lookDir, float fov, float aspect, float znear, float zfar) :
        m_CameraPos(cameraPos), m_LookDir(lookDir), m_Aspect(aspect), m_Znear(znear), m_Zfar(zfar)
    {
        vec3 worldUp(0, 1, 0);
        mat4x4 lookat = LookAt(lookDir, worldUp);
        mat4x4 translateMat = mat4x4::identity();
        translateMat[0][3] = -cameraPos.x;
        translateMat[1][3] = -cameraPos.y;
        translateMat[2][3] = -cameraPos.z;
        m_ViewMatrix = lookat * translateMat;

        m_ProjectionMatrix = PerspProjection(fov, aspect, znear, zfar);
    }

    const vec3 &GetCameraPos() const {
        return m_CameraPos;
    }

    const mat4x4 &GetViewMatrix() const {
        return m_ViewMatrix;
    }
};

#endif // CAMERA_H
