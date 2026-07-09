#pragma once

#include "BVH.hpp"
#include "Intersection.hpp"
#include "Material.hpp"
#include "OBJ_Loader.hpp"
#include "Object.hpp"
#include "Triangle.hpp"
#include <cassert>
#include <array>

inline bool rayTriangleIntersect(const Vector3f& v0, const Vector3f& v1,
                          const Vector3f& v2, const Vector3f& orig,
                          const Vector3f& dir, float& tnear, float& u, float& v)
{
    Vector3f edge1 = v1 - v0;
    Vector3f edge2 = v2 - v0;
    Vector3f pvec = crossProduct(dir, edge2);
    float det = dotProduct(edge1, pvec);
    if (det == 0)
        return false;

    Vector3f tvec = orig - v0;
    u = dotProduct(tvec, pvec);
    if (u < 0 || u > det)
        return false;

    Vector3f qvec = crossProduct(tvec, edge1);
    v = dotProduct(dir, qvec);
    if (v < 0 || u + v > det)
        return false;

    float invDet = 1 / det;

    tnear = dotProduct(edge2, qvec) * invDet;
    u *= invDet;
    v *= invDet;

    return true;
}

class Triangle : public Object
{
public:
    Vector3f v0, v1, v2; // vertices A, B ,C , counter-clockwise order
    Vector3f n0, n1, n2; //新增三个顶点的法线
    Vector3f e1, e2;     // 2 edges v1-v0, v2-v0;
    Vector3f t0, t1, t2; // texture coords
    Vector3f normal;     // 面法线
    Material* m;

    //新构造函数包含顶点
    Triangle(Vector3f _v0, Vector3f _v1, Vector3f _v2, Vector3f _n0, Vector3f _n1, Vector3f _n2, Material* _m = nullptr)
        : v0(_v0), v1(_v1), v2(_v2), n0(_n0), n1(_n1), n2(_n2), m(_m)
    {
        e1 = v1 - v0;
        e2 = v2 - v0;
        normal = normalize(crossProduct(e1, e2));
    }

    //旧构造函数实现向后兼容
    Triangle(Vector3f _v0, Vector3f _v1, Vector3f _v2, Material* _m = nullptr)
        : v0(_v0), v1(_v1), v2(_v2), m(_m)
    {
        e1 = v1 - v0;
        e2 = v2 - v0;
        normal = normalize(crossProduct(e1, e2));
        n0 = n1 = n2 = normal;
    }

    bool intersect(const Ray& ray) override;
    bool intersect(const Ray& ray, float& tnear,
                   uint32_t& index) const override;
    Intersection getIntersection(Ray ray) override;
    void getSurfaceProperties(const Vector3f& P, const Vector3f& I,
                              const uint32_t& index, const Vector2f& uv,
                              Vector3f& N, Vector2f& st) const override
    {
        //N = normal;
        //        throw std::runtime_error("triangle::getSurfaceProperties not
        //        implemented.");
        Vector3f pos = v0 * (1 - uv.x - uv.y) + v1 * uv.x + v2 * uv.y;
        st.x = pos.x * 0.1f;  // 缩放系数控制棋盘格密度
        st.y = pos.y * 0.1f;
    }
    Vector3f evalDiffuseColor(const Vector2f&) const override;
    Bounds3 getBounds() override;
};

class MeshTriangle : public Object
{
public:
    MeshTriangle(const std::string& filename, Material* customMat = nullptr)
    {
        objl::Loader loader;
        loader.LoadFile(filename);

        assert(loader.LoadedMeshes.size() == 1);
        auto mesh = loader.LoadedMeshes[0];

        Vector3f min_vert = Vector3f{std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity(),
                                     std::numeric_limits<float>::infinity()};
        Vector3f max_vert = Vector3f{-std::numeric_limits<float>::infinity(),
                                     -std::numeric_limits<float>::infinity(),
                                     -std::numeric_limits<float>::infinity()};

        Material* baseMat = customMat;
        if (!baseMat) {
            baseMat = new Material(MaterialType::DIFFUSE_AND_GLOSSY,
                                Vector3f(0.5, 0.5, 0.5), Vector3f(0, 0, 0));
            baseMat->Kd = 0.6;
            baseMat->Ks = 0.0;
            baseMat->specularExponent = 0;
        }

        for (int i = 0; i < mesh.Vertices.size(); i += 3) {
            std::array<Vector3f, 3> face_vertices;
            std::array<Vector3f, 3> face_normals; 
            for (int j = 0; j < 3; j++) {
                auto vert = mesh.Vertices[i + j];

                //读取顶点坐标
                auto pos = Vector3f(vert.Position.X, vert.Position.Y, vert.Position.Z) * 60.f;
                face_vertices[j] = pos;
                
                //读取顶点法线
                auto norm = Vector3f(vert.Normal.X, vert.Normal.Y, vert.Normal.Z);
                face_normals[j] = normalize(norm);

                min_vert = Vector3f(std::min(min_vert.x, pos.x),
                                    std::min(min_vert.y, pos.y),
                                    std::min(min_vert.z, pos.z));
                max_vert = Vector3f(std::max(max_vert.x, pos.x),
                                    std::max(max_vert.y, pos.y),
                                    std::max(max_vert.z, pos.z));
            }

            triangles.emplace_back(
                face_vertices[0], face_vertices[1], face_vertices[2],
                face_normals[0], face_normals[1], face_normals[2],
                baseMat   // 使用同一个材质
            );

            // //创建三角形时，传入顶点法线

            // triangles.emplace_back(
            //     face_vertices[0], face_vertices[1], face_vertices[2],
            //     face_normals[0], face_normals[1], face_normals[2],
            //     new_mat
            // );
        }

        bounding_box = Bounds3(min_vert, max_vert);

        std::vector<Object*> ptrs;
        for (auto& tri : triangles)
            ptrs.push_back(&tri);

        bvh = new BVHAccel(ptrs);
    }

    bool intersect(const Ray& ray) { return true; }

    bool intersect(const Ray& ray, float& tnear, uint32_t& index) const
    {
        bool intersect = false;
        for (uint32_t k = 0; k < triangles.size(); k++) {
            /*
            const Vector3f& v0 = vertices[vertexIndex[k * 3]];
            const Vector3f& v1 = vertices[vertexIndex[k * 3 + 1]];
            const Vector3f& v2 = vertices[vertexIndex[k * 3 + 2]];
            */
            const Vector3f v0 = triangles[k].v0;
            const Vector3f v1 = triangles[k].v1;
            const Vector3f v2 = triangles[k].v2;

            float t, u, v;
            if (rayTriangleIntersect(v0, v1, v2, ray.origin, ray.direction, t,
                                     u, v) &&t < tnear) {
                tnear = t;
                index = k;
                intersect = true;
            }
        }

        return intersect;
    }

    Bounds3 getBounds() { return bounding_box; }

    void getSurfaceProperties(const Vector3f& P, const Vector3f& I,
                              const uint32_t& index, const Vector2f& uv,
                              Vector3f& N, Vector2f& st) const
    {
        const Vector3f& v0 = vertices[vertexIndex[index * 3]];
        const Vector3f& v1 = vertices[vertexIndex[index * 3 + 1]];
        const Vector3f& v2 = vertices[vertexIndex[index * 3 + 2]];
        Vector3f e0 = normalize(v1 - v0);
        Vector3f e1 = normalize(v2 - v1);
        N = normalize(crossProduct(e0, e1));
        const Vector2f& st0 = stCoordinates[vertexIndex[index * 3]];
        const Vector2f& st1 = stCoordinates[vertexIndex[index * 3 + 1]];
        const Vector2f& st2 = stCoordinates[vertexIndex[index * 3 + 2]];
        st = st0 * (1 - uv.x - uv.y) + st1 * uv.x + st2 * uv.y;
    }

    Vector3f evalDiffuseColor(const Vector2f& st) const
    {
        float scale = 5;
        float pattern =
            (fmodf(st.x * scale, 1) > 0.5) ^ (fmodf(st.y * scale, 1) > 0.5);
        return lerp(Vector3f(0.815, 0.235, 0.031),
                    Vector3f(0.937, 0.937, 0.231), pattern);
    }

    Intersection getIntersection(Ray ray)
    {
        Intersection intersec;

        if (bvh) {
            intersec = bvh->Intersect(ray);
        }

        return intersec;
    }

    Bounds3 bounding_box;
    std::unique_ptr<Vector3f[]> vertices;
    uint32_t numTriangles;
    std::unique_ptr<uint32_t[]> vertexIndex;
    std::unique_ptr<Vector2f[]> stCoordinates;

    std::vector<Triangle> triangles;

    BVHAccel* bvh;

    Material* m;
};

inline bool Triangle::intersect(const Ray& ray) { return true; }
inline bool Triangle::intersect(const Ray& ray, float& tnear,
                                uint32_t& index) const
{
    return false;
}

inline Bounds3 Triangle::getBounds() { return Union(Bounds3(v0, v1), v2); }

inline Intersection Triangle::getIntersection(Ray ray)
{
    Intersection inter;
    float tnear = std::numeric_limits<float>::max();
    float u, v;

    if (rayTriangleIntersect(v0, v1, v2, ray.origin, ray.direction, tnear, u, v)) {
        inter.happened = true;
        inter.coords = ray.origin + ray.direction * tnear;
        inter.normal = normalize(n0 * (1 - u - v) + n1 * u + n2 * v);
        inter.distance = tnear;
        inter.obj = this;
        inter.m = m;

        // 插值顶点法线
        Vector3f interpolated = normalize(n0 * (1 - u - v) + n1 * u + n2 * v);
        inter.normal = interpolated;
    }

    return inter;
}

inline Vector3f Triangle::evalDiffuseColor(const Vector2f&) const
{
    return Vector3f(0.5, 0.5, 0.5);
}
