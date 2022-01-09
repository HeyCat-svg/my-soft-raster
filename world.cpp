#include "world.h"
#include <algorithm>

World::World() {

}

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
}

void World::Clear(KDNode* node) {
    if (node == nullptr) {
        return;
    }
    Clear(node->left);
    Clear(node->right);
    delete node;
}

bool World::IntersectHelper(const Ray &ray, KDNode *node, HitResult &hitResult, bool shadow) {

}

void World::ReadObjects(const std::string filename) {

}

void World::AddObjects(const Object &obj) {

}

void World::ClearObjects() {

}

void World::Build() {

}

bool World::Intersect(const Ray &ray, HitResult &hitResult, Object &hitObject, bool shadow) {

}

Object& World::GetObjectRef(int i) {

}
