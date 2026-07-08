# Ray Tracing Renderer

一个基于 Whitted 风格的光线追踪渲染器，从零实现主光线生成、三角形求交、BVH 加速结构和 Phong 光照模型。

特性：主光线生成（逐像素生成相机光线，支持透视投影）；三角形求交（Möller–Trumbore 算法，基于重心坐标一次解出交点）；BVH 加速（层次包围盒 + AABB Slab 测试，高效空间剪枝）；Phong 光照（漫反射 + 高光 + 逐光源阴影检测）；Whitted 着色（反射、折射、Fresnel 效应）；模型加载（支持 OBJ 格式）。

快速开始：

编译：
git clone <repository-url>
cd RayTracing/code
mkdir build && cd build
cmake .. && make -j2

运行：
./RayTracing          # BVH 加速版本
./RayTracing check    # 基础检测版本（无加速，用于验证正确性）

程序运行后会在 build/ 目录下生成 binary.ppm 图像文件。

项目结构：
code/
├── BVH.cpp/hpp        BVH 加速结构
├── Bounds3.hpp        AABB 包围盒与 Slab 测试
├── Renderer.cpp/hpp   主渲染器
├── Scene.cpp/hpp      场景管理与着色
├── Triangle.hpp       三角形求交
├── Material.hpp       材质系统
├── main.cpp           程序入口
├── models/            OBJ 模型文件
└── images/            渲染结果

性能测试数据（1280×960 分辨率，Intel Core i7-12700H）：
Stanford Bunny（4,968 面）：暴力求交 7.0 min，BVH 0.45 s，加速比 933×
Stanford Dragon（47,794 面）：暴力求交 8.9 min，BVH 0.39 s，加速比 1,368×
Stanford Dragon 高模（202,520 面）：暴力求交 83.8 min，BVH 0.55 s，加速比 9,143×
Stanford Armadillo（345,944 面）：暴力求交 83.8 min，BVH 0.60 s，加速比 8,364×

核心结论：BVH 将分钟甚至小时级别的暴力求交压缩至亚秒级别，是复杂场景光线追踪可行的关键。

技术栈：C++17，CMake 3.10+，仅依赖 STL。

作者：宋欣悦 · 522031910816
