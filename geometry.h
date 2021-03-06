#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QtCore>
#include <cmath>
#include <cassert>
#include <iostream>

#define PI 3.1415926535897932f

template<int n>
struct vec {
  vec() = default;
  float& operator[](const int i) {assert(i >= 0 && i < n); return data[i];}
  float operator[](const int i) const {assert(i >= 0 && i < n); return data[i];}
  float norm2() const {return (*this)*(*this);}
  float norm() const {return std::sqrt(norm2());}

  float data[n] = {0};
};

template<int n>
vec<n> operator-(const vec<n>& v) {
    vec<n> ret;
    for (int i = n; i--; ret[i] = -v[i]);
    return ret;
}

template<int n>
float operator*(const vec<n>& lhs, const vec<n>& rhs) {
    float ret = 0;
    for (int i = n; i--; ret += lhs[i] * rhs[i]);
    return ret;
}

template<int n>
vec<n> operator+(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] += rhs[i]);
    return ret;
}

template<int n>
vec<n> operator+(const vec<n>& lhs, const float rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] += rhs);
    return ret;
}

template<int n>
vec<n> operator+(const float& lhs, const vec<n> rhs) {
    vec<n> ret = rhs;
    for (int i = n; i--; ret[i] += lhs);
    return ret;
}

template<int n>
vec<n> operator-(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] -= rhs[i]);
    return ret;
}

template<int n>
vec<n> operator*(const float& rhs, const vec<n> &lhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] *= rhs);
    return ret;
}

template<int n>
vec<n> operator*(const vec<n>& lhs, const float& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] *= rhs);
    return ret;
}

template<int n>
vec<n> operator/(const vec<n>& lhs, const float& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] /= rhs);
    return ret;
}

template<int n1, int n2>
vec<n1> embed(const vec<n2> &v, float fill = 1) {
    vec<n1> ret;
    for (int i = n1; i--; ret[i] = (i < n2 ? v[i] : fill));
    return ret;
}

template<int n1, int n2>
vec<n1> proj(const vec<n2> &v) {
    vec<n1> ret;
    for (int i = n1; i--; ret[i] = v[i]);
    return ret;
}

template<int n>
vec<n> mul(const vec<n>& lhs, const vec<n>& rhs) {
    vec<n> ret = lhs;
    for (int i = n; i--; ret[i] *= rhs[i]);
    return ret;
}

template<int n>
vec<n> lerp(const vec<n>& a, const vec<n>& b, float t) {
    return a * (1 - t) + b * t;
}

template<int n>
vec<n> clamp01(vec<n> v) {
    vec<n> ret;
    for (int i = n; i--; ret[i] = (v[i] > 1.f) ? 1.f : ((v[i] < 0.f) ? 0.f : v[i]));
    return ret;
}

template<int n>
std::ostream& operator<<(std::ostream& out, const vec<n>& v) {
    for (int i = 0; i < n; ++i) out << v[i] << " ";
    return out;
}

/////////////////////////////////////////////////////////////////////////////////

template<>
struct vec<2> {
    vec() = default;
    vec(float X, float Y) : x(X), y(Y) {}
    float& operator[](const int i) {assert(i >= 0 && i < 2); return i == 0 ? x : y;}
    float operator[](const int i) const {assert(i >= 0 && i < 2); return i == 0 ? x : y;}
    float norm2() const { return (*this) * (*this) ; }
    float norm() const { return std::sqrt(norm2()); }
    vec& normalize() { *this = (*this) / norm(); return *this; }

    float x{}, y{};
};

/////////////////////////////////////////////////////////////////////////////////

template<>
struct vec<3> {
    vec() = default;
    vec(float X, float Y, float Z) : x(X), y(Y), z(Z) {}
    float& operator[](const int i) {assert(i >= 0 && i < 3); return i == 0 ? x : (1 == i ? y : z);}
    float operator[](const int i) const {assert(i >= 0 && i < 3); return i == 0 ? x : (1 == i ? y : z);}
    float norm2() const {return (*this) * (*this);}
    float norm() const {return std::sqrt(norm2());}
    vec& normalize() {*this = (*this) / norm(); return *this;}

    float x{}, y{}, z{};
};

/////////////////////////////////////////////////////////////////////////////////

template<>
struct vec<4> {
    vec() = default;
    vec(float X, float Y, float Z, float W) : x(X), y(Y), z(Z), w(W) {}
    float& operator[](const int i) {assert(i >= 0 && i < 4); return i == 0 ? x : (1 == i ? y : (2 == i ? z : w));}
    float operator[](const int i) const {assert(i >= 0 && i < 4); return i == 0 ? x : (1 == i ? y : (2 == i ? z : w));}
    float norm2() const {return (*this) * (*this);}
    float norm() const {return std::sqrt(norm2());}
    vec& normalize() {*this = (*this) / norm(); return *this;}

    float x{}, y{}, z{}, w{};
};

/////////////////////////////////////////////////////////////////////////////////

template<int n>
struct dt;

struct Quaternion;

template<int nrows, int ncols>
struct mat {
    vec<ncols> rows[nrows] = {{}};

    mat() = default;
    vec<ncols>& operator[] (const int idx) {assert(idx >= 0 && idx < nrows); return rows[idx];}
    const vec<ncols>& operator[] (const int idx) const {assert(idx >= 0 && idx < nrows); return rows[idx];}

    vec<nrows> col(const int idx) const {
        assert(idx >= 0 && idx < ncols);
        vec<nrows> ret;
        for (int i = nrows; i--; ret[i] = rows[i][idx]);
        return ret;
    }

    void set_col(const int idx, const vec<nrows> &v) {
        assert(idx >= 0 && idx < ncols);
        for (int i = nrows; i--; rows[i][idx] = v[i]);
    }

    static mat<nrows, ncols> identity() {
        mat<nrows, ncols> ret;
        for (int i = nrows; i--; )
            for (int j = ncols; j--; ret[i][j] = (i == j));
        return ret;
    }

    static mat<nrows, ncols> zero() {
        mat<nrows, ncols> ret;
        for (int i = nrows; i--; )
            for (int j = ncols; j--; ret[i][j] = 0);
        return ret;
    }

    // ???????????????
    float det() const {
        return dt<ncols>::det(*this);
    }

    // ????????????
    mat<nrows - 1, ncols - 1> get_minor(const int row, const int col) const {
        mat<nrows - 1, ncols - 1> ret;
        for (int i = nrows - 1; i--; )
            for (int j = ncols - 1; j--; ret[i][j] = rows[i < row ? i : i + 1][j < col ? j : j + 1]);
        return ret;
    }

    // ???????????????
    float cofactor(const int row, const int col) const {
        return get_minor(row, col).det() * ((row + col) % 2 ? -1 : 1);
    }

    // ????????????
    mat<nrows, ncols> adjugate() const {
        mat<nrows, ncols> ret;
        for (int i = nrows; i--; )
            for (int j = ncols; j--; ret[i][j]=cofactor(i,j));
        return ret;
    }

    // ???????????????
    mat<nrows, ncols> invert_transpose() const {
        mat<nrows, ncols> ret = adjugate();
        return ret / (ret[0] * rows[0]);    // ret[0] * rows[0] = (*this).det()
    }

    // ?????????
    mat<nrows, ncols> invert() const {
        return invert_transpose().transpose();
    }

    // ????????????
    mat<ncols, nrows> transpose() const {
        mat<ncols, nrows> ret;
        for (int i = ncols; i--; ret[i] = this->col(i));
        return ret;
    }
};

/////////////////////////////////////////////////////////////////////////////////

template<int nrows, int ncols>
vec<nrows> operator*(const mat<nrows, ncols>& lhs, const vec<ncols>& rhs) {
    vec<nrows> ret;
    for (int i = nrows; i--; ret[i] = lhs[i] * rhs);
    return ret;
}

template<int R1, int C1, int C2>
mat<R1, C2> operator*(const mat<R1, C1>& lhs, const mat<C1, C2>& rhs) {
    mat<R1, C2> result;
    for (int i = R1; i--; )
        for (int j = C2; j--; result[i][j] = lhs[i] * rhs.col(j));
    return result;
}

template<int nrows, int ncols>
mat<nrows, ncols> operator*(const mat<nrows, ncols>& lhs, const float& val) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--; result[i] = lhs[i] * val);
    return result;
}

template<int nrows, int ncols>
mat<nrows, ncols> operator/(const mat<nrows, ncols>& lhs, const float& val) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--; result[i] = lhs[i] / val);
    return result;
}

template<int nrows, int ncols>
mat<nrows, ncols> operator+(const mat<nrows, ncols>& lhs, const mat<nrows, ncols>& rhs) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--; )
        for (int j = ncols; j--; result[i][j] = lhs[i][j] + rhs[i][j]);
    return result;
}

template<int nrows, int ncols>
mat<nrows, ncols> operator-(const mat<nrows, ncols>& lhs, const mat<nrows, ncols>& rhs) {
    mat<nrows, ncols> result;
    for (int i = nrows; i--; )
        for (int j = ncols; j--; result[i][j] = lhs[i][j] - rhs[i][j]);
    return result;
}

template<int nrows, int ncols>
std::ostream& operator<<(std::ostream& out, const mat<nrows,ncols>& m) {
    for (int i = 0; i < nrows; ++i) out << m[i] << std::endl;
    return out;
}

/////////////////////////////////////////////////////////////////////////////////

template<int n>
struct dt {
    // ??????????????????????????????
    static float det(const mat<n, n>& src) {
        float ret = 0;
        for (int i = n; i--; ret += src[0][i] * src.cofactor(0,i));
        return ret;
    }
};

template<>
struct dt<1> {
    // ???????????????????????????
    static float det(const mat<1, 1>& src) {
        return src[0][0];
    }
};

/////////////////////////////////////////////////////////////////////////////////

Quaternion operator*(const Quaternion& lhs, const Quaternion& rhs);

struct Quaternion {
    Quaternion() = default;
    Quaternion(const vec<3>& _v, float _s) : v(_v), s(_s) {}
    Quaternion(const vec<3>& rotation) {
        // y rotation
        Quaternion yaw(std::sin(0.5f * rotation.y) * vec<3>(0, 1, 0), std::cos(0.5f * rotation.y));
        // x rotation
        Quaternion pitch(std::sin(0.5f * rotation.x) * vec<3>(1, 0, 0), std::cos(0.5f * rotation.x));
        // z rotation
        Quaternion roll(std::sin(0.5f * rotation.z) * vec<3>(0, 0, 1), std::cos(0.5f * rotation.z));

        (*this) = roll * pitch * yaw;
    }
    float norm2() const {return v.norm2() + s * s;}
    float norm() const {return std::sqrt(norm2());}
    Quaternion& normalize() {
        float len = norm();
        v = v / len;
        s /= len;
        return *this;
    }

    mat<4, 4> ToMatrix() {
        mat<4, 4> ret;
        ret[0] = vec<4>(1.f - 2.f * v.y * v.y - 2.f * v.z * v.z,
                        2.f * v.x * v.y - 2.f * v.z * s,
                        2.f * v.x * v.z + 2.f * v.y * s,
                        0.f);
        ret[1] = vec<4>(2.f * v.x * v.y + 2.f * v.z * s,
                        1.f - 2.f * v.x * v.x - 2.f * v.z * v.z,
                        2.f * v.y * v.z - 2.f * v.x * s,
                        0.f);
        ret[2] = vec<4>(2.f * v.x * v.y - 2.f * v.y * s,
                        2.f * v.y * v.z + 2.f * v.x * s,
                        1.f - 2.f * v.x * v.x - 2.f * v.y * v.y,
                        0.f);
        ret[3] = vec<4>(0, 0, 0, 1);
        return ret;
    }

    vec<3> v;     // ????????????
    float s;    // ????????????
};

/////////////////////////////////////////////////////////////////////////////////

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;
typedef mat<4, 4> mat4x4;
typedef mat<3, 3> mat3x3;
enum ProjectionType {ORTH, PERSP};

vec3 cross(const vec3& v1, const vec3& v2);
float clamp01(float v);
mat4x4 TRS(vec3& translate, vec3& rotation, vec3& scale);   // ??????MODEL_MATRIX
mat4x4 LookAt(vec3& dir, vec3& up);
mat4x4 Projection(ProjectionType type, float znear, float zfar, float top, float down, float left, float right);
mat4x4 PerspProjection(float fov, float aspect, float znear, float zfar); // fov: ??????????????????

/////////////////////////////////////////////////////////////////////////////////

extern const float MIN;
extern const float MAX;

struct Ray {
    vec3 origin;
    vec3 dir;

    Ray() = default;
    Ray(vec3 o, vec3 d) : origin(o), dir(d) {}
};

/* ???????????????????????????????????????????????????????????? */
struct HitResult {
    Ray ray;
    float t;
    vec3 hitPoint;      // ???????????????
    int hitIdx;         // ???????????????faceID
    vec3 barycentric;   // ????????????????????????????????????????????????
};

struct BoundingBox3f {
    vec3 center;
    vec3 minPoint;
    vec3 maxPoint;

    BoundingBox3f() = default;
    BoundingBox3f(vec3 _minPoint, vec3 _maxPoint) : minPoint(_minPoint), maxPoint(_maxPoint) {
        center = (minPoint + maxPoint) * 0.5f;
    }
    bool Overlaps(const BoundingBox3f& inBox) const {
        return (inBox.minPoint.x >= minPoint.x) && (inBox.minPoint.y >= minPoint.y) && (inBox.minPoint.z >= minPoint.z) &&
               (inBox.maxPoint.x <= maxPoint.x) && (inBox.maxPoint.y <= maxPoint.y) && (inBox.maxPoint.z <= maxPoint.z);
    }
    vec3 GetCenter() const {return center;}

    // ???????????????????????????????????????
    bool Intersect(const Ray& ray, vec2* hitResult = nullptr) const {
        float tMin = MIN, tMax = MAX;

        for (int i = 0; i < 3; ++i) {
            float t1 = (minPoint[i] - ray.origin[i]) / ray.dir[i];
            float t2 = (maxPoint[i] - ray.origin[i]) / ray.dir[i];
            tMin = std::max(tMin, std::min(t1, t2));
            tMax = std::min(tMax, std::max(t1, t2));
        }

        bool intersect = (tMin <= tMax) && (tMax >= 0);

        // ??????????????????????????????
        if (intersect && hitResult != nullptr) {
            (*hitResult)[0] = tMin;
            (*hitResult)[1] = tMax;
        }

        return intersect;
    }

};

/////////////////////////////////////////////////////////////////////////////////

#endif // GEOMETRY_H
