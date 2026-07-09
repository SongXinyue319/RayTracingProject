//
// Created by LEI XU on 5/16/19.
//

#ifndef RAYTRACING_MATERIAL_H
#define RAYTRACING_MATERIAL_H

#include "Vector.hpp"
#include <string>

enum MaterialType { DIFFUSE_AND_GLOSSY, REFLECTION_AND_REFRACTION, REFLECTION };

class Material{
public:
    MaterialType m_type;
    Vector3f m_color;
    Vector3f m_emission;
    float ior;
    float Kd, Ks;
    float specularExponent;
    //Texture tex;
    std::string texturePath;       // 纹理图片路径
    bool hasTexture = false;       // 是否有纹理

    inline Material(MaterialType t=DIFFUSE_AND_GLOSSY, Vector3f c=Vector3f(1,1,1), Vector3f e=Vector3f(0,0,0));
    inline MaterialType getType();
    inline Vector3f getColor();
    inline Vector3f getColorAt(double u, double v);
    inline Vector3f getEmission();

    void loadTexture(const std::string& path);
    Vector3f sampleTexture(const Vector2f& uv) const;
};

Material::Material(MaterialType t, Vector3f c, Vector3f e){
    m_type = t;
    m_color = c;
    m_emission = e;
    Kd = 0.6;
    Ks = 0.0;
    specularExponent = 16;
    ior = 1.3;
    hasTexture = false;
}

MaterialType Material::getType(){return m_type;}
Vector3f Material::getColor(){return m_color;}
Vector3f Material::getEmission() {return m_emission;}

inline Vector3f Material::sampleTexture(const Vector2f& uv) const
{
    if (!hasTexture) {
        return Vector3f(1.0f, 1.0f, 1.0f);
    }
    
    // 棋盘纹理
    int u = (int)(uv.x * 10) % 2;
    int v = (int)(uv.y * 10) % 2;
    if ((u + v) % 2 == 0) {
        return Vector3f(0.8f, 0.2f, 0.2f);  // 红色格子
    } else {
        return Vector3f(0.2f, 0.2f, 0.8f);  // 蓝色格子
    }
}

Vector3f Material::getColorAt(double u, double v) {
    return Vector3f();
}
#endif //RAYTRACING_MATERIAL_H
