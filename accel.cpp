#include "accel.h"
#include <windows.h>
#include <algorithm>

Accel::Accel() {}

Accel::Accel(Model* mesh) : m_Mesh(mesh) {}

void Accel::Split(KDNode *node, int depth) {
    m_MaxDepth = std::max(m_MaxDepth, depth);
    int nface = node->tris.size();
    if (nface < m_SplitTermination) {
        return;
    }

    // 对三角形数组排序
    PartitionRule rule = node->rule;        // 向下划分的依据
    switch (rule) {
    case X_AXIS:
        std::sort(node->tris.begin(), node->tris.end(), [this](const int &l, const int &r) {
            return this->m_Mesh->GetBoundingBox(l).GetCenter().x < this->m_Mesh->GetBoundingBox(r).GetCenter().x;
        });
        break;
    case Y_AXIS:
        std::sort(node->tris.begin(), node->tris.end(), [this](const int &l, const int &r) {
            return this->m_Mesh->GetBoundingBox(l).GetCenter().y < this->m_Mesh->GetBoundingBox(r).GetCenter().y;
        });
        break;
    case Z_AXIS:
        std::sort(node->tris.begin(), node->tris.end(), [this](const int &l, const int &r) {
            return this->m_Mesh->GetBoundingBox(l).GetCenter().z < this->m_Mesh->GetBoundingBox(r).GetCenter().z;
        });
        break;
    }

    // 重建左右包围盒
    vec3 minVertLeft(MAX, MAX, MAX), maxVertLeft(MIN, MIN, MIN);
    vec3 minVertRight(MAX, MAX, MAX), maxVertRight(MIN, MIN, MIN);
    // 重建左节点的包围盒
    for (int i = 0; i < nface / 2; ++i) {
        const BoundingBox3f& box = m_Mesh->GetBoundingBox(node->tris[i]);
        for (int j = 0; j < 3; ++j) {
            minVertLeft[j] = std::min(minVertLeft[j], box.minPoint[j]);
            maxVertLeft[j] = std::max(maxVertLeft[j], box.maxPoint[j]);
        }
    }
    // 重建右节点的包围盒
    for (int i = nface / 2; i < nface; ++i) {
        const BoundingBox3f& box = m_Mesh->GetBoundingBox(node->tris[i]);
        for (int j = 0; j < 3; ++j) {
            minVertRight[j] = std::min(minVertRight[j], box.minPoint[j]);
            maxVertRight[j] = std::max(maxVertRight[j], box.maxPoint[j]);
        }
    }

    // 新建左右分支
    node->left = new KDNode(BoundingBox3f(minVertLeft, maxVertLeft), (PartitionRule)(((int)rule + 1) % 3));
    node->left->tris = std::vector<int>(node->tris.begin(), node->tris.begin() + nface / 2);
    node->right = new KDNode(BoundingBox3f(minVertRight, maxVertRight), (PartitionRule)(((int)rule + 1) % 3));
    node->right->tris = std::vector<int>(node->tris.begin() + nface / 2, node->tris.end());

    // 清空该节点
    node->tris.clear();
    node->tris.shrink_to_fit();

    // 递归分裂左右子节点
    m_LeafNum ++;
    m_NodeNum += 2;
    Split(node->left, depth + 1);
    Split(node->right, depth + 1);
}

void Accel::SetMesh(Model* mesh) {
    m_Mesh = mesh;
}

void Accel::Build() {
    if (m_Mesh == nullptr) {
        return;
    }

    // timer start
    LARGE_INTEGER cpuFreq;
    LARGE_INTEGER startTime;
    LARGE_INTEGER endTime;
    double runtime = 0.0;
    QueryPerformanceFrequency(&cpuFreq);
    QueryPerformanceCounter(&startTime);

    Clear();
    m_TreeRoot = new KDNode(m_Mesh->GetBoundingBox(), X_AXIS);
    int nface = m_Mesh->nfaces();
    for (int i = 0; i < nface; ++i) {
        m_TreeRoot->tris.emplace_back(i);   // 先将所有的三角形放在一个node里
    }
    m_NodeNum = 1;
    m_LeafNum = 1;
    m_MaxDepth = 1;
    Split(m_TreeRoot, 1);                   // 然后递归划分

    // timer end
    QueryPerformanceCounter(&endTime);
    runtime = (((endTime.QuadPart - startTime.QuadPart) * 1000.0f) / cpuFreq.QuadPart);
    qDebug() << "build time: " << runtime << "ms";
    qDebug() << "depth: " << m_MaxDepth << "\tnode num: " << m_NodeNum << "\tdleaf num: " << m_LeafNum;
}

void Accel::Clear(KDNode* node) {
    if (node == nullptr) {
        return;
    }
    Clear(node->left);
    Clear(node->right);
    delete node;
}

void Accel::Clear() {
    Clear(m_TreeRoot);
    m_TreeRoot = nullptr;
}

bool Accel::IntersectHelper(const Ray &ray, KDNode* node, HitResult &hitResult, bool shadow) {
    if (!node->boundingBox.Intersect(ray)) {
        return false;
    }

    // 叶子节点
    if (node->left == nullptr && node->right == nullptr) {
        bool hit = false;
        float tMin = MAX;
        int hitIdx;
        vec3 barycentric;
        int nface = node->tris.size();
        for (int i = 0; i < nface; ++i) {
            float t;
            vec3 bar;
            if (m_Mesh->Intersect(node->tris[i], ray, bar, t) && t < tMin) {
                if (shadow) {   // 检测shadow的时候不需要知道光线碰撞点信息 只需要知道光线有没有被遮挡
                    return true;
                }
                tMin = t;
                barycentric = bar;
                hitIdx = node->tris[i];
                hit = true;
            }
        }
        hitResult.barycentric = barycentric;
        hitResult.hitIdx = hitIdx;
        hitResult.t = tMin;
        return hit;
    }

    // 非叶子节点
    hitResult.t = MAX;      // 防止检测右节点时(tmpResult.t < hitResult.t)出现bug
    HitResult tmpResult;
    bool hitLeft, hitRight;
    if ((hitLeft = IntersectHelper(ray, node->left, tmpResult, shadow))) {
        if (shadow) {
            return true;
        }
        hitResult = tmpResult;
    }
    if ((hitRight = IntersectHelper(ray, node->right, tmpResult, shadow))) {
        if (shadow) {
            return true;
        }
        if (tmpResult.t < hitResult.t) {
            hitResult = tmpResult;
        }
    }
    return hitLeft || hitRight;
}

bool Accel::Intersect(const Ray& ray, HitResult& hitResult, bool shadow) {
    bool ret = IntersectHelper(ray, m_TreeRoot, hitResult, shadow);
    if (ret) {
        hitResult.ray = ray;
        hitResult.hitPoint = ray.origin + hitResult.t * ray.dir;
    }
    return ret;
}
