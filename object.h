#ifndef OBJECT_H
#define OBJECT_H

#include "geometry.h"
#include "model.h"
#include "accel.h"


// 一个Object包含一个mesh和一个材质
class Object {
    Model* m_Model;                     // 不需要obj来回收 内存分配在外部完成
    Accel* m_AccelStruct;               // 不需要obj来回收 内存分配在外部完成 build也在外部完成
    BRDFMaterial* m_Material;           // 不需要obj来回收 记录了brdf函数

    BoundingBox3f m_ObjBoundingBox;                 // obj在空间的AABB包围盒
    mat4x4 m_ModelMatrix, m_ModelMatrixInverse;     // model->local to world
    vec3 m_T, m_R, m_S;                             // translate rotation scale
    bool m_IsLight = false;                         // 通过model名字中是否包含light判断是否光源
    float m_LightArea = -1.f;                       // world space光源面积 只有在m_IsLight=true时有效

public:
    Object() = default;
    Object(Model* model, Accel* accelStruct, BRDFMaterial* material,
           vec3 T=vec3(0, 0, 0), vec3 R=vec3(0, 0, 0), vec3 S=vec3(1, 1, 1));
    ~Object();
    void SetTRS(vec3 T, vec3 R, vec3 S);
    const BRDFMaterial& GetMaterial() const;
    const BoundingBox3f& GetBoundingBox() const;
    bool Intersect(const Ray& ray, HitResult& hitResult, bool shadow = false);
    bool GetLight(float& lightArea);

    int nverts() const;
    int nfaces() const;
    vec3 normal(const int iface, const int nthvert) const;
    vec3 vert(const int i) const;
    vec3 vert(const int iface, const int nthvert) const;
    vec2 uv(const int iface, const int nthvert) const;
};

#endif // OBJECT_H
