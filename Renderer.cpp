//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#ifdef _OPENMP
#include <omp.h>
#endif


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene,bool check_mode)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    Vector3f eye_pos(-1, 5, 10);
    
    int totalRows = scene.height;
    int completedRows = 0;
    int lastPercent = 0;

    #pragma omp parallel for
    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {
            const int SAMPLES = 8;
            Vector3f color(0,0,0);

            for (int s = 0; s < SAMPLES; s++){
                unsigned int seed = i + j * scene.width + s * 1000;
                float offsetX = ((float)rand_r(&seed) / RAND_MAX - 0.5f) * 0.5f;
                float offsetY = ((float)rand_r(&seed) / RAND_MAX - 0.5f) * 0.5f;

                float x = (2.0f * (i + offsetX + 0.5f) / scene.width - 1.0f) * imageAspectRatio * scale;
                float y = (1.0f - 2.0f * (j + offsetY + 0.5f) / scene.height) * scale;
                Vector3f dir = normalize(Vector3f(x, y, -1));
                Ray ray(eye_pos, dir);
                
                if (check_mode)
                    color += scene.castRay_noBVH(ray, 0);
                else
                    color += scene.castRay(ray, 0);
            }

            framebuffer[j * scene.width + i] = color / SAMPLES;
        }

        #pragma omp atomic
        completedRows++;

        #pragma omp critical
        {
            int percent = (int)((float)completedRows / (float)scene.height * 100);
            if (percent > lastPercent) {
                UpdateProgress((float)percent / 100.0f);
                lastPercent = percent;
            }
        }
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = fopen("binary.ppm", "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].x));
        color[1] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].y));
        color[2] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].z));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);
}
