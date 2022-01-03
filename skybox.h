#ifndef SKYBOX_H
#define SKYBOX_H

#include <string>

#include "geometry.h"
#include "tgaimage.h"

class Skybox {
    int width, height;

    TGAImage *X_POSI = nullptr, *X_NEGA = nullptr;
    TGAImage *Y_POSI = nullptr, *Y_NEGA = nullptr;
    TGAImage *Z_POSI = nullptr, *Z_NEGA = nullptr;

public:
    Skybox() = default;
    Skybox(const std::string filename);
    ~Skybox();

    void ReadTextures(const std::string filename);  // 文件名格式特定 参数filename是前缀
    vec3 GetColor(vec3 dir);                  // 根据向量对天空盒进行采样
};

#endif // SKYBOX_H
