#ifndef SHADER_H
#define SHADER_H

#include "geometry.h"
#include "tgaimage.h"

///////////////////////////////////////// SHADER ENV ////////////////////////////

extern mat4x4 MODEL_MATRIX;     // model to world
extern mat4x4 VIEW_MATRIX;      // world to view
extern mat4x4 PROJ_MATRIX;      // view to clip space

void SetModelMatrix(mat4x4 mat);
void SetViewMatrix(mat4x4 mat);
void SetProjectionMatrix(mat4x4 mat);

/////////////////////////////////////////////////////////////////////////////////

class IShader {
public:
    virtual ~IShader() {};
    virtual vec3 Vertex(int iface, int nthvert) = 0;
    virtual bool Fragment(vec3 barycentric, TGAColor& outColor);
};

class GeneralShader : public IShader {
    struct v2f {

    };

    v2f vertOutput[3];

 public:
    virtual vec3 Vertex(int iface, int nthvert) override {


        return vec3();
    }

    virtual bool Fragment(vec3 barycentric, TGAColor& outColor) override {


        return false;
    }
};

#endif // SHADER_H
