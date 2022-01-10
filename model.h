#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include <string>
#include "geometry.h"
#include "tgaimage.h"
#include "material.h"

class Object;

class Model {
    friend class Object;
private:
    std::vector<vec3> verts_;     // array of vertices
    std::vector<vec2> uv_;        // array of tex coords
    std::vector<vec3> norms_;     // array of normal vectors
    std::vector<int> facet_vrt_;
    std::vector<int> facet_tex_;  // indices in the above arrays per triangle
    std::vector<int> facet_nrm_;
    BoundingBox3f model_bounding_box_;      // 模型包围盒
    BoundingBox3f** tri_bounding_box_;      // 三角形包围盒
    std::string name;

public:
    Model() = default;
    Model(const std::string filename);
    ~Model();
    int nverts() const;
    int nfaces() const;
    vec3 normal(const int iface, const int nthvert) const;  // per triangle corner normal vertex
    vec3 vert(const int i) const;
    vec3 vert(const int iface, const int nthvert) const;
    vec2 uv(const int iface, const int nthvert) const;

    const std::string& GetName();
    const BoundingBox3f& GetBoundingBox(int faceIdx) const;
    const BoundingBox3f& GetBoundingBox() const;        // TODO: 需要换成模型的多边形凸包
    void InitBoundingBox();             // 初始化模型local包围盒 同时为tri_bounding_box_分配内存

    // 计算光线和模型的某个三角面片的交点 bar是重心坐标
    bool Intersect(int faceIdx, const Ray& ray, vec3& bar, float& t);

    // 用于读取有多个对象的obj文件
    static std::vector<Model*> ModelReader(const std::string filename);
};

#endif // MODEL_H
