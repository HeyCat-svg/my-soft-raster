#ifndef LIGHT_H
#define LIGHT_H

#include "geometry.h"

class Light {

    mat4x4 m_ViewMatrix;
    mat4x4 m_ProjectionMatrix;
    vec3 m_LightColor;
    vec3 m_LightDir;
    vec3 m_LightPos;
    ProjectionType m_ProjectionType;

public:
    Light(vec3 lightColor, vec3 lightPos, vec3 lightDir, ProjectionType projType) :
        m_LightColor(lightColor), m_LightPos(lightPos), m_LightDir(lightDir)
    {
        vec3 worldUp(0, 1, 0);
        mat4x4 lookat = LookAt(lightDir, worldUp);
        mat4x4 translateMat = mat4x4::identity();
        translateMat[0][3] = -lightPos.x;
        translateMat[1][3] = -lightPos.y;
        translateMat[2][3] = -lightPos.z;
        m_ViewMatrix = lookat * translateMat;

        if (projType == ProjectionType::PERSP) {
            m_ProjectionMatrix = PerspProjection(PI / 3.f, 1.f, 0.3f, 10.f);
        }
        else if (projType == ProjectionType::ORTH) {
            m_ProjectionMatrix = Projection(ProjectionType::ORTH, 0.3f, 10.f, 5.f, -5.f, -5.f, 5.f);
        }
    }

    // 没有经过z反转的view矩阵
    const mat4x4 &GetViewMatrix() const {
        return m_ViewMatrix;
    }

    const mat4x4 GetProjectionMatrix() const {
        return m_ProjectionMatrix;
    }
};

#endif // LIGHT_H
