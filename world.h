#ifndef WORLD_H
#define WORLD_H

#include "geometry.h"
#include "object.h"
#include <vector>
#include <string>


// 用于记录世界中的光源信息
struct WorldLight {
    float m_LightAera;                          // 光源面积
    vec3 m_LightStartPointAndDir[3];            // [0]:start point [1],[2]:dir1,dir2
    vec3 m_LightNormal;                           // 矩形光源的法向
    const BRDFMaterial* m_LightMaterial;        // 使用emission计算光强
};

class World {
    struct KDNode {
        BoundingBox3f boundingBox;
        std::vector<int> objs;      // 记录world中该node所包含的obj序号
        KDNode* left = nullptr;
        KDNode* right = nullptr;

        KDNode(BoundingBox3f _box, KDNode* _left = nullptr, KDNode* _right = nullptr) {
            boundingBox = _box;
            left = _left;
            right = _right;
        }
    };

private:
    std::vector<Object> m_Objects;          // 该世界中含有的obj列表
    std::vector<WorldLight> m_WorldLights;  // world中矩形light列表
    KDNode* m_TreeRoot = nullptr;
    int m_MaxDepth = 0, m_LeafNum = 0, m_NodeNum = 0;
    int m_SplitTermination = 1;             // node的obj数量小于等于该数目时停止分裂

    void Split(KDNode* node, int depth);
    void Clear(KDNode* node);
    bool IntersectHelper(const Ray& ray, KDNode* node, HitResult& hitResult, int& hitObjIdx, bool shadow);

public:
    World();
    void AddObjects(const Object& obj);
    void ClearAccel();                              // 删除加速结构
    void Build();                                   // 重建加速结构
    bool Intersect(const Ray& ray, HitResult& hitResult, Object& hitObject, bool shadow = false);
    Object& GetObjectRef(int i);                    // 获取世界列表中某一个Obj的引用
    const std::vector<WorldLight>& GetLights() const;           // 获取世界光照
};

#endif // WORLD_H
