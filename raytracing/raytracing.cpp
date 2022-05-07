//
// Created by numi on 5/6/22.
//

#include <iostream>
#include "raytracing.h"

bool Intersection(const Sphere& sphere, const Vec3& start, const Vec3& ray, float* result) {
    const Vec3& o = start - sphere.center;
    const Vec3& v = ray;
    const float radius = sphere.radius;
    const float quad_discr = (o * v) * (o * v) - (v * v) * ((o * o) - radius * radius);
    if (quad_discr < 0) return false;
    if (result == nullptr) return true;

    *result = (-(o * v) - sqrtf(quad_discr)) / (v * v);
    return true;
}

void PrintVec(const Vec3& vec) {
    std::cout << vec.x << ", "
              << vec.y << ", "
              << vec.z << '\n';
}

void PrintColor(const Color& color) {
    std::cout << color.red << ", "
              << color.green << ", "
              << color.blue << '\n';
}

// find closest primitive that is intersected by ray
// excluding ignored_primitive
bool FindPrimitive(
        const Vec3& start,
        const Vec3& ray,
        const std::vector<Sphere>& spheres,
        float* min_intersection,
        int* index,
        int ignored_primitive = -1 //index of primitive that is ignored when finding the next one
) {
    float min = INFINITY;
    int idx = -1;
    for (int i = 0; i < spheres.size(); i++) {
        if (i == ignored_primitive) continue;
        float intersection;
        if (Intersection(spheres[i], start, ray, &intersection)) {
            if (intersection < 0) continue;
            if (min > intersection) {
                idx = i;
                min = intersection;
            }
        }
    }

    *min_intersection = min;
    *index = idx;

    return idx >= 0;
}

Color CalculateIntensity(
        const Vec3& start,
        const Vec3& ray,
        const std::vector<Light>& light_sources,
        const std::vector<Sphere>& spheres,
        const Color& ambient,
        int sphere_index,
        int depth = 0 // reflection depth
) {
    if (sphere_index < 0) {
        // we can't get intensities below zero if we calculate it from different sources
        // so we can just set these pixels to background after finding maximum intensity
        return Color {-1.0f, -1.0f, -1.0f };
    }

    const Sphere& sphere = spheres[sphere_index];
    const Vec3 intersection = start + ray;
    const Vec3 normal = (intersection - sphere.center).norm();
    const Vec3 view = (ray * -1).norm();

    Color intensity = sphere.diffuse * ambient;
    for (const auto& light: light_sources) {
        const Vec3 light_vec =  light.position - intersection;
        // check if the object is facing the light in this point
        const Vec3 light_norm = light_vec.norm();
        float light_cosine = (normal * light_norm);
        if (light_cosine < 0) continue;

        // check if the light isn't hidden by other objects
        bool is_hidden = false;
        for (int i = 0; i < spheres.size(); i++) {
            if (i == sphere_index) continue;
            float result;
            if (Intersection(spheres[i], light.position, light_vec * -1, &result)) {
                if (result < 1.0f) {
                    is_hidden = true;
                    break;
                }
            }
        }
        if (is_hidden) continue;

        // add intensity from light
        const float reflect_cosine = powf(light_vec.reflection(normal) * view, 100);
        intensity += light.color * (sphere.diffuse * light_cosine + sphere.specular * reflect_cosine) * light_vec.f_att();
    }

    // add intensity reflected from other objects
    if (depth > 0) {
        int index;
        float min;
        const Vec3 new_ray = ray.reflection(normal) * -1;
        if (FindPrimitive(intersection, new_ray, spheres, &min, &index, sphere_index)) {
            const Color reflected_intensity = CalculateIntensity(
                    intersection, new_ray * min,
                    light_sources, spheres,
                    ambient, index,
                    depth - 1
            );
            intensity += sphere.specular * reflected_intensity * (new_ray * min).f_att();
        }
    }

    return intensity;
}


struct PixelSamples {
    Color colors[4];
};

// Traces rays through pixels and determines the color by applying light sources and reflection
// Puts all the pixels into image
void Raytracing(const Camera& camera,
                const std::vector<Light>& light_sources,
                const std::vector<Sphere>& spheres,
                int* image,
                const Color& background,
                const Color& ambient
) {
    const int width = camera.sw;
    const int height = camera.sh;
    std::vector<PixelSamples> intensities(width * height);

    const Vec3 center = camera.z.norm() * camera.zn;
    const Vec3 dx = camera.right.norm() * 0.5;
    const Vec3 dy = camera.up.norm() * -0.5;
    const Vec3 start = camera.eye;
    const Vec3 start_ray = center
            + dx * (-width + 0.5f)
            + dy * (-height - 0.5f);

    for (int y = 0; y < 2 * height; y++) {
        const Vec3 row_ray = start_ray + dy * y;
        for (int x = 0; x < 2 * width; x++) {
            const int pixel_index = width * (y / 2) + (x / 2);
            const int sample_index = 2 * (y % 2) + (x % 2);
            const Vec3 ray = row_ray + dx * x;

            int index;
            float min_intersection;
            FindPrimitive(start, ray, spheres, &min_intersection, &index);
            intensities[pixel_index].colors[sample_index] = CalculateIntensity(
                    start, ray * min_intersection,
                    light_sources, spheres,
                    ambient, index,
                    3
                    );
        }
    }

    // convert all components from [0, max_intensity] to [0, 1] and then to int rgba
    float max_intensity = 0;
    for (int i = 0; i < width * height; i++) {
        for (int j = 0; j < 4; j++) {
            Color& intensity = intensities[i].colors[j];
            if (max_intensity < intensity.red) {
                max_intensity = intensity.red;
            }

            if (max_intensity < intensity.green) {
                max_intensity = intensity.green;
            }

            if (max_intensity < intensity.blue) {
                max_intensity = intensity.blue;
            }
        }
    }

    for (int i = 0; i < width * height; i++) {
        Color sum {0, 0, 0};
        for (auto & color : intensities[i].colors) {
            if (color.red < 0) {
                sum += background;
            } else {
                sum += color / max_intensity;
            }
        }
        image[i] = (sum / 4).rgba();
    }
}

