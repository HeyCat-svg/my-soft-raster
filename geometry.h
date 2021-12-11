#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <QtCore>
#include <cmath>
#include <cassert>
#include <iostream>

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

template<int n>
struct dt;

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

    // 行列式求值
    float det() const {
        return dt<ncols>::det(*this);
    }

    // 余子矩阵
    mat<nrows - 1, ncols - 1> get_minor(const int row, const int col) const {
        mat<nrows - 1, ncols - 1> ret;
        for (int i = nrows - 1; i--; )
            for (int j = ncols - 1; j--; ret[i][j] = rows[i < row ? i : i + 1][j < col ? j : j + 1]);
        return ret;
    }

    // 代数余子式
    float cofactor(const int row, const int col) const {
        return get_minor(row, col).det() * ((row + col) % 2 ? -1 : 1);
    }

    // 伴随矩阵
    mat<nrows, ncols> adjugate() const {
        mat<nrows, ncols> ret;
        for (int i = nrows; i--; )
            for (int j = ncols; j--; ret[i][j]=cofactor(i,j));
        return ret;
    }

    // 逆转置矩阵
    mat<nrows, ncols> invert_transpose() const {
        mat<nrows, ncols> ret = adjugate();
        return ret / (ret[0] * rows[0]);    // ret[0] * rows[0] = (*this).det()
    }

    // 逆矩阵
    mat<nrows, ncols> invert() const {
        return invert_transpose().transpose();
    }

    // 转置矩阵
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
    // 求矩阵对应的行列式值
    static double det(const mat<n, n>& src) {
        float ret = 0;
        for (int i = n; i--; ret += src[0][i] * src.cofactor(0,i));
        return ret;
    }
};

template<>
struct dt<1> {
    // 求行列式的递归末尾
    static float det(const mat<1, 1>& src) {
        return src[0][0];
    }
};

/////////////////////////////////////////////////////////////////////////////////

typedef vec<2> vec2;
typedef vec<3> vec3;
typedef vec<4> vec4;
vec3 cross(const vec3& v1, const vec3& v2);

#endif // GEOMETRY_H
