//
// Created by numi on 5/6/22.
//

#ifndef UNTITLED_RAYTRACING_H
#define UNTITLED_RAYTRACING_H

#include <vector>
#include <cmath>

#include "imgui.h" //for color macros

struct Color {
    float red, green, blue;
    int rgba() const {
        return IM_COL32(
        (int) (red * 255),
        (int) (green * 255),
        (int) (blue * 255),
        255);
    }

    Color operator*(const Color& other) const {
        return Color {red * other.red, green * other.green, blue * other.blue};
    }

    Color operator*(float factor) const {
        return Color {red * factor, green * factor, blue * factor};
    }

    Color operator/(const Color& other) const {
        return Color {red / other.red, green / other.green, blue / other.blue};
    }

    Color operator/(float factor) const {
        return *this * (1 / factor);
    }

    Color& operator+=(const Color& other) {
        red += other.red;
        green += other.green;
        blue += other.blue;
        return *this;
    }

    Color operator+(const Color& other) const {
        return Color {red + other.red, green + other.green, blue + other.blue};
    }
};

struct Vec3 {
    float x, y, z;
    Vec3 operator-(const Vec3& other) const {
        return Vec3 {x - other.x, y - other.y, z - other.z};
    }

    Vec3 operator+(const Vec3& other) const {
        return Vec3 {x + other.x, y + other.y, z + other.z};
    }

    Vec3 operator*(float factor) const {
        return Vec3 {x * factor, y * factor, z * factor};
    }

    Vec3 operator/(float factor) const {
        return *this * (1 / factor);
    }

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    [[nodiscard]] Vec3 cross(const Vec3& other) const {
        return Vec3 {
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
        };
    }

    [[nodiscard]] float operator*(const Vec3& other) const {
        return x * other.x + y * other.y + z * other.z;
    }

    [[nodiscard]] float length() const {
        return sqrtf(x * x + y * y + z * z);
    }

    [[nodiscard]] Vec3 norm() const {
        return *this / length();
    }

    [[nodiscard]] Vec3 reflection(const Vec3& normal) const {
        const Vec3 proj = normal * (normal * *this);
        const Vec3 tangent = *this - proj;
        return (proj + (tangent * -1)).norm();
    }

    [[nodiscard]] float f_att() const {
        return 1 / (1 + length() * 0.001);
    }
};

struct Sphere {
    Vec3 center;
    float radius;
    Color diffuse; //=ambient
    Color specular;
};

struct Light {
    Vec3 position;
    Color color;
};

struct Camera {
    Camera(const Vec3& eye,
           const Vec3& view,
           const Vec3& up,
           float zn, float zf,
           int sw, int sh){
        this->zn = zn;
        this->zf = zf;
        this->sw = sw;
        this->sh = sh;
        this->z = view - eye;
        this->eye = eye;
        this->right = z.cross(up);
        this->up = right.cross(z);
    }

    Vec3 eye;
    Vec3 z;
    Vec3 right;
    Vec3 up;
    float zn, zf;
    int sw, sh;
};

void Raytracing(const Camera& camera,
                const std::vector<Light>& light_sources,
                const std::vector<Sphere>& spheres,
                int* image, //sw x sh
                const Color& background = Color {0, 0, 0},
                const Color& ambient = Color {1, 1, 1}
                );

bool Intersection(const Sphere& sphere, const Vec3& start, const Vec3& ray, float* result);

#endif //UNTITLED_RAYTRACING_H
