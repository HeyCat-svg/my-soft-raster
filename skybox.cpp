#include "skybox.h"

Skybox::Skybox(const std::string filename) {
    X_POSI = new TGAImage();
    X_POSI->read_tga_file(filename + "_x+.tga");
    X_NEGA = new TGAImage();
    X_NEGA->read_tga_file(filename + "_x-.tga");
    Y_POSI = new TGAImage();
    Y_POSI->read_tga_file(filename + "_y+.tga");
    Y_NEGA = new TGAImage();
    Y_NEGA->read_tga_file(filename + "_y-.tga");
    Z_POSI = new TGAImage();
    Z_POSI->read_tga_file(filename + "_z+.tga");
    Z_NEGA = new TGAImage();
    Z_NEGA->read_tga_file(filename + "_z-.tga");

    width = X_POSI->get_width();
    height = X_POSI->get_height();

    // 对不同轴的贴图进行不同的反转操作 因为击中不同轴其坐标系不一样
    X_POSI->flip_horizontally();
    X_POSI->flip_vertically();

    X_NEGA->flip_vertically();

    Y_NEGA->flip_vertically();

    Z_POSI->flip_vertically();

    Z_NEGA->flip_horizontally();
    Z_NEGA->flip_vertically();

}

Skybox::~Skybox() {
    delete X_POSI;
    delete X_NEGA;
    delete Y_POSI;
    delete Y_NEGA;
    delete Z_POSI;
    delete Z_NEGA;
}

void Skybox::ReadTextures(const std::string filename) {
    if (X_POSI == nullptr) {
        X_POSI = new TGAImage();
    }
    X_POSI->read_tga_file(filename + "_x+.tga");

    if (X_NEGA == nullptr) {
        X_NEGA = new TGAImage();
    }
    X_NEGA->read_tga_file(filename + "_x-.tga");

    if (Y_POSI == nullptr) {
        Y_POSI = new TGAImage();
    }
    Y_POSI->read_tga_file(filename + "_y+.tga");

    if (Y_NEGA == nullptr) {
        Y_NEGA = new TGAImage();
    }
    Y_NEGA->read_tga_file(filename + "_y-.tga");

    if (Z_POSI == nullptr) {
        Z_POSI = new TGAImage();
    }
    Z_POSI->read_tga_file(filename + "_z+.tga");

    if (Z_NEGA == nullptr) {
        Z_NEGA = new TGAImage();
    }
    Z_NEGA->read_tga_file(filename + "_z-.tga");
}

vec3 Skybox::GetColor(vec3 dir) {
    float absX = std::abs(dir.x);
    float absY = std::abs(dir.y);
    float absZ = std::abs(dir.z);

    if (absX > absY && absX > absZ) {   // 击中X+或X-
        if (dir.x > 0) {                // 击中X+
            dir = dir / dir.x;
            dir = dir * 0.5f + 0.5f;
            TGAColor col = X_POSI->get(dir.z * width, dir.y * height);
            return vec3(col[2] / 255.f, col[1] / 255.f, col[0] / 255.f);
        }
        else {                          // 击中X-
            dir = dir / -dir.x;
            dir = dir * 0.5f + 0.5f;
            TGAColor col = X_NEGA->get(dir.z * width, dir.y * height);
            return vec3(col[2] / 255.f, col[1] / 255.f, col[0] / 255.f);
        }
    }
    else if (absY > absZ) {             // 击中Y+或Y-
        if (dir.y > 0) {                // 击中Y+
            dir = dir / dir.y;
            dir = dir * 0.5f + 0.5f;
            TGAColor col = Y_POSI->get(dir.x * width, dir.z * height);
            return vec3(col[2] / 255.f, col[1] / 255.f, col[0] / 255.f);
        }
        else {                          // 击中Y-
            dir = dir / -dir.y;
            dir = dir * 0.5f + 0.5f;
            TGAColor col = Y_NEGA->get(dir.x * width, dir.z * height);
            return vec3(col[2] / 255.f, col[1] / 255.f, col[0] / 255.f);
        }
    }
    else {                              // 击中Z+或Z-
        if (dir.z > 0) {                // 击中Z+
            dir = dir / dir.z;
            dir = dir * 0.5f + 0.5f;
            TGAColor col = Z_POSI->get(dir.x * width, dir.y * height);
            return vec3(col[2] / 255.f, col[1] / 255.f, col[0] / 255.f);
        }
        else {                          // 击中Z-
            dir = dir / -dir.z;
            dir = dir * 0.5f + 0.5f;
            TGAColor col = Z_NEGA->get(dir.x * width, dir.y * height);
            return vec3(col[2] / 255.f, col[1] / 255.f, col[0] / 255.f);
        }
    }
}
