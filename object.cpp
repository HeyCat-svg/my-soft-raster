#include "object.h"

Object::Object(Model* model, Accel* accelStruct, BRDFMaterial* material,
               vec3 T, vec3 R, vec3 S) :
    m_Model(model), m_AccelStruct(accelStruct), m_Material(material), m_T(T), m_R(R), m_S(S)
{
    // 初始化model等矩阵
    m_ModelMatrix = TRS(T, R, S);
    m_ModelMatrixInverse = m_ModelMatrix.invert();

    // 构造world空间的AABB包围盒
    const BoundingBox3f& localBox = model->GetBoundingBox();
    vec3 boxPts[2];
    boxPts[0] = localBox.minPoint;
    boxPts[1] = localBox.maxPoint;
    vec3 minPoint(MAX, MAX, MAX);
    vec3 maxPoint(MIN, MIN, MIN);
    for (int i = 0; i < 8; ++i) {
        vec4 boxPoint(boxPts[i / 4].x, boxPts[i % 4 / 2].y, boxPts[i % 2].z, 1.f);
        boxPoint = m_ModelMatrix * boxPoint;    // 包围盒顶点local to world
        minPoint.x = std::min(boxPoint.x, minPoint.x);
        minPoint.y = std::min(boxPoint.y, minPoint.y);
        minPoint.z = std::min(boxPoint.z, minPoint.z);

        maxPoint.x = std::max(boxPoint.x, maxPoint.x);
        maxPoint.y = std::max(boxPoint.y, maxPoint.y);
        maxPoint.z = std::max(boxPoint.z, maxPoint.z);
    }
    m_ObjBoundingBox = BoundingBox3f(minPoint, maxPoint);
}

Object::~Object() {

}

void Object::SetTRS(vec3 T, vec3 R, vec3 S) {
    // 初始化model等矩阵
    m_ModelMatrix = TRS(T, R, S);
    m_ModelMatrixInverse = m_ModelMatrix.invert();

    // 构造world空间的AABB包围盒
    const BoundingBox3f& localBox = m_Model->GetBoundingBox();
    vec3 boxPts[2];
    boxPts[0] = localBox.minPoint;
    boxPts[1] = localBox.maxPoint;
    vec3 minPoint(MAX, MAX, MAX);
    vec3 maxPoint(MIN, MIN, MIN);
    for (int i = 0; i < 8; ++i) {
        vec4 boxPoint(boxPts[i / 4].x, boxPts[i % 4 / 2].y, boxPts[i % 2].z, 1.f);
        boxPoint = m_ModelMatrix * boxPoint;    // 包围盒顶点local to world
        minPoint.x = std::min(boxPoint.x, minPoint.x);
        minPoint.y = std::min(boxPoint.y, minPoint.y);
        minPoint.z = std::min(boxPoint.z, minPoint.z);

        maxPoint.x = std::max(boxPoint.x, maxPoint.x);
        maxPoint.y = std::max(boxPoint.y, maxPoint.y);
        maxPoint.z = std::max(boxPoint.z, maxPoint.z);
    }
    m_ObjBoundingBox = BoundingBox3f(minPoint, maxPoint);
}

const BRDFMaterial& Object::GetMaterial() const {
    return *m_Material;
}

const BoundingBox3f& Object::GetBoundingBox() const {
    return m_ObjBoundingBox;
}

bool Object::Intersect(const Ray &ray, HitResult &hitResult, bool shadow) {
    // 首先将ray从world to local
    Ray localRay;
    localRay.origin = proj<3>(m_ModelMatrixInverse * embed<4>(ray.origin));
    localRay.dir = proj<3>(m_ModelMatrixInverse * embed<4>(ray.dir, 0.f)).normalize();

    // 然后在local space碰撞检测
    if (m_AccelStruct->Intersect(localRay, hitResult, shadow)) {
        if (shadow) {
            return true;
        }
        // 将local space的碰撞点转换到world space
        hitResult.hitPoint = proj<3>(m_ModelMatrix * embed<4>(hitResult.hitPoint));
        hitResult.ray = ray;
        return true;
    }
    return false;
}

int Object::nverts() const {

}
