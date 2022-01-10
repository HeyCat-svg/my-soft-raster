#include "world.h"
#include <algorithm>

World::World() {}

void World::Split(KDNode* node, int depth) {
    m_MaxDepth = std::max(m_MaxDepth, depth);
    int nobj = node->objs.size();
    if (nobj <= m_SplitTermination) {
        return;
    }

    const BoundingBox3f& box = node->boundingBox;
    float dx = box.maxPoint.x - box.minPoint.x;
    float dy = box.maxPoint.y - box.minPoint.y;
    float dz = box.maxPoint.z - box.minPoint.z;

    if (dx > dy && dx > dz) {
        std::sort(node->objs.begin(), node->objs.end(), [this](const int &l, const int &r){
           return this->m_Objects[l].GetBoundingBox().GetCenter().x < this->m_Objects[r].GetBoundingBox().GetCenter().x;
        });
    }
    else if (dy > dz) {
        std::sort(node->objs.begin(), node->objs.end(), [this](const int &l, const int &r){
           return this->m_Objects[l].GetBoundingBox().GetCenter().y < this->m_Objects[r].GetBoundingBox().GetCenter().y;
        });
    }
    else {
        std::sort(node->objs.begin(), node->objs.end(), [this](const int &l, const int &r){
           return this->m_Objects[l].GetBoundingBox().GetCenter().z < this->m_Objects[r].GetBoundingBox().GetCenter().z;
        });
    }

    // 重建左右包围盒
    vec3 minVertLeft(MAX, MAX, MAX), maxVertLeft(MIN, MIN, MIN);
    vec3 minVertRight(MAX, MAX, MAX), maxVertRight(MIN, MIN, MIN);
    // 重建左节点的包围盒
    for (int i = 0; i < nobj / 2; ++i) {
        const BoundingBox3f& box = m_Objects[node->objs[i]].GetBoundingBox();
        for (int j = 0; j < 3; ++j) {
            minVertLeft[j] = std::min(minVertLeft[j], box.minPoint[j]);
            maxVertLeft[j] = std::max(maxVertLeft[j], box.maxPoint[j]);
        }
    }
    // 重建右节点包围盒
    for (int i = nobj / 2; i < nobj; ++i) {
        const BoundingBox3f& box = m_Objects[node->objs[i]].GetBoundingBox();
        for (int j = 0; j < 3; ++j) {
            minVertRight[j] = std::min(minVertRight[j], box.minPoint[j]);
            maxVertRight[j] = std::max(maxVertRight[j], box.maxPoint[j]);
        }
    }

    // 新建左右分支
    node->left = new KDNode(BoundingBox3f(minVertLeft, maxVertLeft));
    node->left->objs = std::vector<int>(node->objs.begin(), node->objs.begin() + nobj / 2);
    node->right = new KDNode(BoundingBox3f(minVertRight, maxVertRight));
    node->right->objs = std::vector<int>(node->objs.begin() + nobj / 2, node->objs.end());

    // 清空该节点
    node->objs.clear();
    node->objs.shrink_to_fit();

    // 递归分裂左右子节点
    m_LeafNum++;
    m_NodeNum += 2;
    Split(node->left, depth + 1);
    Split(node->right, depth + 1);
}

void World::Clear(KDNode* node) {
    if (node == nullptr) {
        return;
    }
    Clear(node->left);
    Clear(node->right);
    delete node;
    node = nullptr;
}

bool World::IntersectHelper(const Ray &ray, KDNode *node, HitResult &hitResult, int& hitObjIdx, bool shadow) {
    if (!node->boundingBox.Intersect(ray)) {
        return false;
    }

    // 叶子节点
    if (node->left == nullptr && node->right == nullptr) {
        bool hit = false;
        float tMin = MAX;
        int nobj = node->objs.size();
        for (int i = 0; i < nobj; ++i) {
            Object& obj = m_Objects[node->objs[i]];
            if (obj.Intersect(ray, hitResult, shadow)) {
                if (shadow) {
                    return true;
                }
                if (hitResult.t < tMin) {
                    tMin = hitResult.t;
                    hitObjIdx = node->objs[i];
                    hit = true;
                }
            }
        }
        return hit;
    }

    // 非叶子节点
    hitResult.t = MAX;
    HitResult tmpResult;
    int tmpHitObjIdx = -1;
    bool hitLeft, hitRight;
    if ((hitLeft = IntersectHelper(ray, node->left, tmpResult, tmpHitObjIdx, shadow))) {
        if (shadow) {
            return true;
        }
        hitResult = tmpResult;
        hitObjIdx = tmpHitObjIdx;
    }
    if ((hitRight = IntersectHelper(ray, node->right, tmpResult, tmpHitObjIdx, shadow))) {
        if (shadow) {
            return true;
        }
        if (tmpResult.t < hitResult.t) {
            hitResult = tmpResult;
            hitObjIdx = tmpHitObjIdx;
        }
    }
    return hitLeft || hitRight;
}

void World::AddObjects(const Object &obj) {
    m_Objects.emplace_back(obj);
}

void World::ClearAccel() {
    Clear(m_TreeRoot);
}

void World::Build() {
    ClearAccel();

    // 计算所有obj的包围盒
    vec3 minVert(MAX, MAX, MAX), maxVert(MIN, MIN, MIN);
    int nobj = m_Objects.size();
    for (int i = 0; i < nobj; ++i) {
        const BoundingBox3f& box = m_Objects[i].GetBoundingBox();
        for (int j = 0; j < 3; ++j) {
            minVert[j] = std::min(minVert[j], box.minPoint[j]);
            maxVert[j] = std::max(maxVert[j], box.maxPoint[j]);
        }
    }

    m_TreeRoot = new KDNode(BoundingBox3f(minVert, maxVert));
    for (int i = 0; i < nobj; ++i) {
        m_TreeRoot->objs.emplace_back(i);
    }
    m_NodeNum = 1;
    m_LeafNum = 1;
    m_MaxDepth = 1;
    Split(m_TreeRoot, 1);
}

bool World::Intersect(const Ray &ray, HitResult &hitResult, Object &hitObject, bool shadow) {
    int hitObjIdx = -1;
    if (IntersectHelper(ray, m_TreeRoot, hitResult, hitObjIdx, shadow)) {
        if (shadow) {
            return true;
        }
        hitObject = m_Objects[hitObjIdx];
        return true;
    }
    return false;
}

Object& World::GetObjectRef(int i) {
    int size = m_Objects.size();
    if (i >= 0 && i < size) {
        return m_Objects[i];
    }
    return m_Objects[0];
}
