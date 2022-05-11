//
// Created by numi on 5/6/22.
//

#ifndef UNTITLED_RAYTRACING_H
#define UNTITLED_RAYTRACING_H

#include <vector>
#include <cmath>
#include <memory>

#include "imgui.h" //for color macros

struct Color {
    float red = 0, green = 0, blue = 0;
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

    Color& operator*=(const Color& other) {
        red *= other.red;
        green *= other.green;
        blue *= other.blue;
        return *this;
    }

    Color operator+(const Color& other) const {
        return Color {red + other.red, green + other.green, blue + other.blue};
    }
};

struct Vec3 {
    float x = 0, y = 0, z = 0;
    [[nodiscard]] Vec3 operator-(const Vec3& other) const {
        return Vec3 {x - other.x, y - other.y, z - other.z};
    }

    [[nodiscard]] Vec3 operator+(const Vec3& other) const {
        return Vec3 {x + other.x, y + other.y, z + other.z};
    }

    [[nodiscard]] Vec3 operator*(float factor) const {
        return Vec3 {x * factor, y * factor, z * factor};
    }

    [[nodiscard]] Vec3 operator/(float factor) const {
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
        return 1 / (1 + length() * 0.0005);
    }
};

struct Material {
    Color diffuse; //=ambient
    Color specular;
    float power = 0;
};

class Primitive {
public:
    virtual Vec3 Normal(const Vec3& intersection) const = 0;
    virtual bool Intersection(const Vec3& start, const Vec3& ray, float* result) const = 0;
    virtual ~Primitive() = default;
    virtual const Material& material() const = 0;
};

class Sphere : public Primitive {
private:
    Vec3 center;
    float radius = 0;
    Material _material;
public:
    Sphere(const Vec3& center, float radius, const Material& material): center {center}, radius {radius}, _material {material} {}
    [[nodiscard]] const Material& material() const override { return _material; }

    bool Intersection(const Vec3 &start, const Vec3 &ray, float *result) const override {
        const Vec3& o = start - center;
        const Vec3& v = ray;
        const float quad_discr = (o * v) * (o * v) - (v * v) * ((o * o) - radius * radius);
        if (quad_discr < 0) return false;

        *result = (-(o * v) - sqrtf(quad_discr)) / (v * v);
        return true;
    }

    [[nodiscard]] Vec3 Normal(const Vec3 &intersection) const override {
        return (intersection - center).norm();
    }
};

// returns k for which (start + k * ray, normal) = 0
float OrthogonalEquation(const Vec3& start, const Vec3& ray, const Vec3& normal);

class Triangle : public Primitive {
private:
    Vec3 a, b, c;
    Vec3 normal;
    Vec3 ab_normal, bc_normal, ca_normal;
    Material _material;
    bool exclude_line;
public:
    // a, b, c are clockwise
    // normal is (c - a) x (b - a)
    Triangle(const Vec3& a, const Vec3& b, const Vec3& c, const Material& material, bool exclude_line = false):
            a {a}, b {b}, c {c},
            normal { (c - a).cross(b - a).norm() },
            ab_normal { normal.cross(b - a) },
            bc_normal { normal.cross(c - b) },
            ca_normal { normal.cross(a - c) },
            _material {material},
            exclude_line {exclude_line} {}
    [[nodiscard]] const Material& material() const override { return _material; }

    bool Intersection(const Vec3 &start, const Vec3 &ray, float *result) const override {
        const float k = -((start - a) * normal) / (normal * ray);
        const Vec3 point = start + ray * k;
        float res = OrthogonalEquation(a - c, point - c, ab_normal);
        if (res < 1 || (exclude_line && res == 1)) return false;
        res = OrthogonalEquation(b - a, point - a, bc_normal);
        if (res < 1 || (exclude_line && res == 1)) return false;
        res = OrthogonalEquation(c - b, point - b, ca_normal);
        if (res < 1 || (exclude_line && res == 1)) return false;
        *result = k;
        return true;
    }

    [[nodiscard]] Vec3 Normal(const Vec3 &intersection) const override {
        return normal;
    }
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
                const std::vector<std::unique_ptr<Primitive>>& primitives,
                int* image, //sw x sh,
                int depth = 1,
                const Color& background = Color {0, 0, 0},
                const Color& ambient = Color {1, 1, 1}
                );

#endif //UNTITLED_RAYTRACING_H
