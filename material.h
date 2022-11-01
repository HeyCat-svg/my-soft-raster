#ifndef MATERIAL_H
#define MATERIAL_H

#include "geometry.h"


class BRDFMaterial {
public:
    // all the direction is point to outside from the fragment
    virtual ~BRDFMaterial() = 0;
    virtual vec3 BRDF(const vec3& rayIn, const vec3& rayOut, const vec3& n) const = 0;
    virtual vec3 Emision(const vec3& rayOut, float distance) const = 0;
};


// TODO: 后续更新为光强随着距离而衰减
class PointLightBRDF : public BRDFMaterial {
    vec3 m_LightColor;
    float m_LightIntensity;
    float m_ZeroPoint = 5.f;

public:
    PointLightBRDF(vec3 lightColor, float lightIntensity) :
        m_LightColor(lightColor), m_LightIntensity(lightIntensity)
    {}

    ~PointLightBRDF() {}

    virtual vec3 BRDF(const vec3& rayIn, const vec3& rayOut, const vec3& n) const override {
        return vec3(0, 0, 0);
    }

    virtual vec3 Emision(const vec3& rayOut, float distance) const override {
        vec3 zero(0, 0, 0);
        if (distance > m_ZeroPoint) {
            return zero;
        }
        float t = distance / m_ZeroPoint;
        return lerp(m_LightIntensity * m_LightColor, zero, t * t);
    }
};

class OpaqueBRDF : public BRDFMaterial {
    float m_Metallicness;
    float m_Smoothness;     // 百分比光滑度
    vec3 m_Albedo;          // 材质自身的反照率 但实际镜面高光颜色和diffuse颜色要根据metalness计算

    float m_Roughness;      // (1-smoothness)^2
    // 当金属度为0的时候的f0 即垂直看表面的光反射率 可以看成镜面高光 金属度越高高光颜色越接近mainTex本色
    const vec4 m_ColorSpaceDielectricSpec = {0.04f, 0.04f, 0.04f, 1.f - 0.04f};

    // d90是垂直表面看材质的反照率(应该写成f0)
    vec3 Schlick_F(vec3 d90, float NdotV) const {
        vec3 unit = vec3(1.f, 1.f, 1.f);
        return d90 + (unit - d90) * std::pow(1.f - NdotV, 5);
    }

    float GGX_D(float roughness, float NdotH) const {
        float a2 = roughness * roughness;
        float d = NdotH * NdotH * (a2 - 1.f) + 1.f;
        return a2 / (PI * (d * d + 1e-7));
    }

    float CookTorrence_G(float NdotL, float NdotV, float VdotH, float NdotH) const {
        float G1 = 2.f * NdotH * NdotV / VdotH;
        float G2 = 2.f * NdotH * NdotL / VdotH;
        return std::min(1.f, std::min(G1, G2));
    }

    float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float perceptualRoughness) const {
        float fd90 = 0.5f + 2.f * LdotH * LdotH * perceptualRoughness;
        float lightScatter = 1.f + (fd90 - 1.f) * std::pow(1 - NdotL, 5);
        float viewScatter = 1.f + (fd90 - 1.f) * std::pow(1 - NdotV, 5);
        return lightScatter * viewScatter;
    }

public:
    OpaqueBRDF(vec3 albedo, float metallicness, float smoothness) :
        m_Metallicness(metallicness), m_Smoothness(smoothness), m_Albedo(albedo)
    {
        m_Roughness = std::pow(1.f - smoothness, 2);
    }

    ~OpaqueBRDF() {}

    virtual vec3 BRDF(const vec3& rayIn, const vec3& rayOut, const vec3& n) const override {
        vec3 halfDir = (rayIn + rayOut).normalize();
        float NdotL = clamp01(n * rayIn);
        float NdotH = clamp01(n * halfDir);
        float NdotV = clamp01(n * rayOut);
        float VdotH = clamp01(rayOut * halfDir);
        float LdotH = clamp01(rayIn * halfDir);

        // 根据metalness算实际镜面高光反照率和漫反射反照率
        vec3 specColor = lerp(proj<3>(m_ColorSpaceDielectricSpec), m_Albedo, m_Metallicness);
        // reflectivity = lerp(0.04, 1, metalness)  ~Spec.w = 1 - 0.04
        float oneMinusReflectivity = m_ColorSpaceDielectricSpec.w * (1.f - m_Metallicness);
        vec3 diffColor = oneMinusReflectivity * m_Albedo;

        vec3 diff = diffColor * DisneyDiffuse(NdotV, NdotL, LdotH, 1.f - m_Smoothness);
        float D = GGX_D(m_Roughness, NdotH);
        vec3 F = Schlick_F(specColor, NdotV);
        float G = CookTorrence_G(NdotL, NdotV, VdotH, NdotH);
        vec3 spec = (D * F * G) * PI / (4.f * (NdotL * NdotV));

        return diff + spec;
    }

    virtual vec3 Emision(const vec3& rayOut, float distance) const override {
        return vec3(0, 0, 0);
    }
};

#endif // MATERIAL_H
