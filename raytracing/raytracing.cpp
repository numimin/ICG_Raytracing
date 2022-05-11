#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
//
// Created by numi on 5/6/22.
//

#include <iostream>
#include "raytracing.h"

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
        const std::vector<std::unique_ptr<Primitive>>& primitives,
        float* min_intersection,
        int* index,
        int ignored_primitive = -1 //index of primitive that is ignored when finding the next one
) {
    float min = INFINITY;
    int idx = -1;
    for (int i = 0; i < primitives.size(); i++) {
        if (i == ignored_primitive) continue;
        float intersection;
        if (primitives[i]->Intersection(start, ray, &intersection)) {
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

// Returns true if there are other primitives in front of primitives[index] in path of light,
// where start + ray is intersection of light with primitives[index], ray is light direction
bool IsHidden(
        const Vec3& start,
        const Vec3& ray,
        const std::vector<std::unique_ptr<Primitive>>& primitives,
        int index
) {
    for (int i = 0; i < primitives.size(); i++) {
        if (i == index) continue;
        float result;
        if (primitives[i]->Intersection(start, ray, &result)) {
            if (result <= 1.0f) {
                return true;
            }
        }
    }
    return false;
}

Color CalculateIntensity(
        const Vec3& start,
        const Vec3& ray,
        const std::vector<Light>& light_sources,
        const std::vector<std::unique_ptr<Primitive>>& primitives,
        const Color& ambient,
        int primitive_index,
        int depth = 0 // reflection depth
) {
    if (primitive_index < 0) {
        // we can't get intensities below zero if we calculate it from different sources
        // so we can just set these pixels to background after finding maximum intensity
        return Color {-1.0f, -1.0f, -1.0f };
    }

    Vec3 intersection = start + ray;

    Color intensity {0, 0, 0};
    Color reflection_coefficient {1, 1, 1};
    for (int i = 0; i < depth + 1; i++) {
        const Primitive& primitive = *primitives[primitive_index];

        Color reflected_intensity = primitive.material().diffuse * ambient;

        const Vec3 normal = primitive.Normal(intersection);
        const Vec3 view = (ray * -1).norm();

        for (const auto& light: light_sources) {
            const Vec3 light_vec = light.position - intersection;
            // check if the object is facing the light in this point
            const Vec3 light_norm = light_vec.norm();
            float light_cosine = normal * light_norm;
            if (light_cosine < 0) continue;

            if (IsHidden(light.position, light_vec * -1, primitives, primitive_index)) {
                continue;
            }

            // add intensity from light
            const float reflect_cosine = light_vec.reflection(normal) * view;
            const Color specular = reflect_cosine > 0 ? primitive.material().specular * powf(reflect_cosine, primitive.material().power) : Color {};
            reflected_intensity += light.color * (primitive.material().diffuse * light_cosine + specular) * light_vec.f_att();
        }

        intensity += reflection_coefficient * reflected_intensity;

        // find light from other objects
        if (i != depth) {
            const Vec3 new_ray = ray.reflection(normal) * -1;
            float min_intersection;
            if (!FindPrimitive(intersection, new_ray, primitives, &min_intersection, &primitive_index, primitive_index)) {
                break;
            }
            intersection += new_ray * min_intersection;
            reflection_coefficient *= primitive.material().specular * (new_ray * min_intersection).f_att();
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
                const std::vector<std::unique_ptr<Primitive>>& primitives,
                int* image,
                int depth,
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

    #pragma omp parallel for
    for (int y = 0; y < 2 * height; y++) {
        const Vec3 row_ray = start_ray + dy * y;
        for (int x = 0; x < 2 * width; x++) {
            const int pixel_index = width * (y / 2) + (x / 2);
            const int sample_index = 2 * (y % 2) + (x % 2);
            const Vec3 ray = row_ray + dx * x;

            int index;
            float min_intersection;
            FindPrimitive(start, ray, primitives, &min_intersection, &index);
            intensities[pixel_index].colors[sample_index] = CalculateIntensity(
                    start, ray * min_intersection,
                    light_sources, primitives,
                    ambient, index,
                    depth
            );
        }
    }

    // convert all components from [0, max_intensity] to [0, 1] and then to int rgba
    float max_intensity = 0;
    for (int i = 0; i < width * height; i++) {
        for (auto & intensity : intensities[i].colors) {
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

#pragma clang diagnostic pop