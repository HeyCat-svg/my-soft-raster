#ifndef ACCEL_H
#define ACCEL_H

#include "geometry.h"
#include "model.h"
#include <vector>

enum PartitionRule {X_AXIS=0, Y_AXIS, Z_AXIS};

struct KDNode {
    BoundingBox3f boundingBox;
    std::vector<int> tris;
    KDNode* left = nullptr;
    KDNode* right = nullptr;

    PartitionRule rule;     // 如果左右子节点非空 则该字段表示划分成两个子节点的依据

    KDNode(BoundingBox3f _box, PartitionRule _rule, KDNode* _left = nullptr, KDNode* _right = nullptr) {
        boundingBox = _box;
        rule = _rule;
        left = _left;
        right = _right;
    }
};

class Accel {
    Model* m_Mesh = nullptr;

private:
    KDNode* m_TreeRoot = nullptr;
    int m_MaxDepth = 0, m_LeafNum = 0, m_NodeNum = 0;
    int m_SplitTermination = 5;     // 当叶子节点的三角形数量小于此数量时停止分裂

    void Split(KDNode* node, int depth);
    void Clear(KDNode* node);
    bool IntersectHelper(const Ray& ray, KDNode* node, HitResult& hitResult, bool shadow);

public:
    Accel();
    Accel(Model* mesh);

    void SetMesh(Model* mesh);
    void Build();
    void Clear();
    bool Intersect(const Ray& ray, HitResult& hitResult, bool shadow = false);
};

#endif // ACCEL_H
