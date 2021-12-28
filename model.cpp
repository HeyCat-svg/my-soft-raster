#include <iostream>
#include <fstream>
#include <sstream>
#include "model.h"

Model::Model(const std::string filename) : verts_(), uv_(), norms_(), facet_vrt_(), facet_tex_(), facet_nrm_() {
    vec3 minVert(MAX, MAX, MAX);
    vec3 maxVert(MIN, MIN, MIN);

    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            vec3 v;
            for (int i=0;i<3;i++) {
                iss >> v[i];
                minVert[i] = std::min(minVert[i], v[i]);
                maxVert[i] = std::max(maxVert[i], v[i]);
            }

            verts_.push_back(v);
        } else if (!line.compare(0, 3, "vn ")) {
            iss >> trash >> trash;
            vec3 n;
            for (int i=0;i<3;i++) iss >> n[i];
            norms_.push_back(n.normalize());
        } else if (!line.compare(0, 3, "vt ")) {
            iss >> trash >> trash;
            vec2 uv;
            for (int i=0;i<2;i++) iss >> uv[i];
            uv_.push_back(uv);
        }  else if (!line.compare(0, 2, "f ")) {
            int f,t,n;
            iss >> trash;
            int cnt = 0;
            while (iss >> f >> trash >> t >> trash >> n) {
                facet_vrt_.push_back(--f);
                facet_tex_.push_back(--t);
                facet_nrm_.push_back(--n);
                cnt++;
            }
            if (3!=cnt) {
                std::cerr << "Error: the obj file is supposed to be triangulated" << std::endl;
                in.close();
                return;
            }
        }
    }
    in.close();
    model_bounding_box_ = BoundingBox3f(minVert, maxVert);
    tri_bounding_box_ = new BoundingBox3f*[nfaces()];
    memset(tri_bounding_box_, 0, sizeof(BoundingBox3f*) * nfaces());
    std::cerr << "# v# " << nverts() << " f# "  << nfaces() << " vt# " << uv_.size() << " vn# " << norms_.size() << std::endl;
}

Model::~Model() {
    int nface = nfaces();
    for (int i = 0; i < nface; ++i) {
        if (tri_bounding_box_[i] != nullptr) {
            delete tri_bounding_box_[i];
        }
    }
    delete[] tri_bounding_box_;
}

int Model::nverts() const {
    return verts_.size();
}

int Model::nfaces() const {
    return facet_vrt_.size()/3;
}

vec3 Model::vert(const int i) const {
    return verts_[i];
}

vec3 Model::vert(const int iface, const int nthvert) const {
    return verts_[facet_vrt_[iface*3+nthvert]];
}

vec2 Model::uv(const int iface, const int nthvert) const {
    return uv_[facet_tex_[iface*3+nthvert]];
}

vec3 Model::normal(const int iface, const int nthvert) const {
    return norms_[facet_nrm_[iface*3+nthvert]];
}

const BoundingBox3f& Model::GetBoundingBox(int faceIdx) const {
    if (tri_bounding_box_[faceIdx] != nullptr) {
        return *(tri_bounding_box_[faceIdx]);
    }

    vec3 verts[3];
    vec3 minVert, maxVert;
    for (int i = 0; i < 3; ++i) {
        verts[i] = verts_[facet_vrt_[faceIdx * 3 + i]];
    }
    minVert.x = std::min(verts[0].x, std::min(verts[1].x, verts[2].x));
    minVert.y = std::min(verts[0].y, std::min(verts[1].y, verts[2].y));
    minVert.z = std::min(verts[0].z, std::min(verts[1].z, verts[2].z));

    maxVert.x = std::max(verts[0].x, std::max(verts[1].x, verts[2].x));
    maxVert.y = std::max(verts[0].y, std::max(verts[1].y, verts[2].y));
    maxVert.z = std::max(verts[0].z, std::max(verts[1].z, verts[2].z));

    tri_bounding_box_[faceIdx] = new BoundingBox3f(minVert, maxVert);

    return *(tri_bounding_box_[faceIdx]);
}

const BoundingBox3f& Model::GetBoundingBox() const {
    return model_bounding_box_;
}

// 见GAMES101 Lec13
bool Model::Intersect(int faceIdx, const Ray& ray, vec3& bar, float& t) {
    vec3 verts[3];
    for (int i = 0; i < 3; ++i) {
        verts[i] = verts_[facet_vrt_[faceIdx * 3 + i]];
    }

    vec3 v01 = verts[1] - verts[0];
    vec3 v02 = verts[2] - verts[0];
    vec3 s = ray.origin - verts[0];
    vec3 s1 = cross(ray.dir, v02);
    vec3 s2 = cross(s, v01);

    float prefix = 1.f / (s1 * v01);
    bar.y = prefix * (s1 * s);
    bar.z = prefix * (s2 * ray.dir);
    bar.x = 1.f - bar.y - bar.z;

    if (bar.x < 0 || bar.y < 0 || bar.z < 0 || t < 0) {
        return false;
    }

    t = prefix * (s2 * v02);
    return true;
}
