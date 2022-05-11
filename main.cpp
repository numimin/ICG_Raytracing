#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <omp.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "raytracing.h"

void error_callback(int error, const char* description) {
    std::cerr << "Error: " << description << '\n';
}

constexpr int image_width = 720;
constexpr int image_height = 480;

void UpdateTexture(GLuint texture_id, void* image, int width, int height) {
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, image);
}

GLuint LoadSampleTexture(void* image, int width, int height) {
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    return image_texture;
}

constexpr float angles_to_radians = M_PI / 180.0;

struct Scene {
    std::vector<std::unique_ptr<Primitive>> primitives = {};
    std::vector<Light> sources = {};
    Vec3 eye { 0, -image_height * 0.5 * 0.5, 0 };
    Vec3 view { 0, -image_height * 0.5 * 0.5, image_width / 4.0f };
    Vec3 up { 0, 1, 0 };
    float zn = image_width * 0.05;
    float zf = 5 * image_width;
    Color background {0.0, 0.0, 0.0};
    Color ambient {0.01, 0.01, 0.01};
    int depth = 1;
    float zoom_factor = 1.0;
    float azimuth = 0.0;
    float attitude = 0.0;

    [[nodiscard]] Camera camera() const {
        const float radius = (view - eye).length();
        const Vec3 z = (eye - view).norm();
        const Vec3 right = z.cross(up).norm();
        const Vec3 up = right.cross(z).norm();
        return Camera {
                view + (
                        (z * cosf(azimuth * angles_to_radians) + right * sinf(azimuth * angles_to_radians)) * cosf(attitude * angles_to_radians)
                        + up * sinf(attitude * angles_to_radians)
                        ) * (radius / zoom_factor),
                view, up,
                zn, zf,
                image_width, image_height
        };
    }
};

struct Box {
    Vec3 center;
    float width, height, distance;
    Vec3 x, y, z;
    Vec3 half_x() const {
        return x * (width / 2);
    }

    Vec3 half_y() const {
        return y * (height / 2);
    }

    Vec3 half_z() const {
        return z * (distance / 2);
    }

    Vec3 at(float x = 0, float y = 0, float z = 0) const {
        return center + half_x() * x + half_y() * y + half_z() * z;
    }
};

void FillSquare(std::vector<std::unique_ptr<Primitive>>& primitives,
                const Material& material,
                const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d,
                bool first_exclude = false
) {
    primitives.push_back(std::make_unique<Triangle>(a, b, c, material, first_exclude));
    primitives.push_back(std::make_unique<Triangle>(a, c, d, material, !first_exclude));
}

// Fills a box opened from front plane (orthogonal to z, with minimum z)
void FillBoxScene(std::vector<std::unique_ptr<Primitive>>& primitives,
                  std::vector<Light>& sources,
                  const Box& box,
                  const Material& material
) {
    const Vec3 back_low_left = box.at(-1, -1, -1);
    const Vec3 back_up_left = box.at(-1, 1, -1);
    const Vec3 back_up_right = box.at(1, 1, -1);
    const Vec3 back_low_right = box.at(1, -1, -1);

    const Vec3 front_low_left = box.at(-1, -1, 1);
    const Vec3 front_up_left = box.at(-1, 1, 1);
    const Vec3 front_up_right = box.at(1, 1, 1);
    const Vec3 front_low_right = box.at(1, -1, 1);

    FillSquare(primitives,
               material,
               back_low_left,
               back_up_left,
               back_up_right,
               back_low_right
    );
    FillSquare(primitives,
               Material {
        // Mirror
        Color {0, 0, 0},  Color {1, 1, 1}, 20
        },
               front_low_left,
               front_up_left,
               back_up_left,
               back_low_left,
               true
    );
    // Water tank
    FillSquare(primitives,
               Material {
                       Color {0, 0, 1}, Color {0.9, 0.9, 0.9}, 20
               },
               back_low_right,
               back_up_right,
               front_up_right,
               front_low_right
    );
    FillSquare(primitives,
               material,
               back_up_left,
               front_up_left,
               front_up_right,
               back_up_right
    );
    FillSquare(primitives,
               material,
               front_low_left,
               back_low_left,
               back_low_right,
               front_low_right
    );
}

void FillScene(
        std::vector<std::unique_ptr<Primitive>>& primitives,
        std::vector<Light>& sources
) {
    primitives.push_back(std::make_unique<Triangle>(
            Vec3 {image_width * 3, -image_height * 3, 4 * image_width},
            Vec3 {0, image_height * 3, 5 * image_width},
            Vec3 {-image_width * 3, -image_height * 3, 5 * image_width},
            Material {
                    Color { 0.9, 0.9, 0.9 },
                    Color { 1, 1, 1 },
                    100
            }
    ));
    primitives.push_back(std::make_unique<Sphere>(
            Vec3 { -image_width, 0, image_width * 3 },
            image_width / 2.0f,
            Material {
                    Color { 0.1, 0.1, 0.9 },
                    Color { 0, 0, 0 },
                    100
            }
    ));
    primitives.push_back(std::make_unique<Sphere>(
                              Vec3 { -image_width * 0.7f, image_height * 1.5, image_width * 4 },
                              image_width / 2.0f,
                              Material {
                                      Color { 0.5, 0.1, 0.9 },
                                      Color { 1, 1, 1 },
                                      100
                              }
                      ));
    primitives.push_back(std::make_unique<Sphere>(
                              Vec3 { 0, 0, image_width * 3 },
                              image_width / 2.0f,
                              Material {
                                      Color { 0.658, 0.658, 0.658 },
                                      Color { 0.658, 0.658, 0.658 },
                                      150
                              }
                      ));

    primitives.push_back(std::make_unique<Sphere>(
                              Vec3 { image_width, 0, image_width * 2 },
                              image_width / 2.0f,
                              Material {
                                      Color { 1, 1, 1 },
                                      Color { 0, 0, 0 },
                                      0
                              }
                      ));

    sources.push_back({
                              Vec3 {-image_width, image_width, image_width},
                              Color { 1.0, 1.0, 1.0 }
                      });

    sources.push_back({
                              Vec3 {-image_width, -image_width, image_width},
                              Color { 1.0, 1.0, 1.0 }
                      });
    sources.push_back({
                              Vec3 {image_width, -image_width, image_width},
                              Color { 1.0, 1.0, 1.0 }
                      });
    sources.push_back({
                              Vec3 {image_width, image_width, image_width},
                              Color { 1.0, 1.0, 1.0 }
                      });
    sources.push_back({
                              Vec3 {0, 0, image_width},
                              Color { 1.0, 1.0, 1.0 }
                      });

    sources.push_back({
                              Vec3 {0, 0, image_width},
                              Color { 1.0, 1.0, 1.0 }
                      });

    sources.push_back({
                              Vec3 {image_width - 250, 0, image_width * 2.5 },
                              Color { 1.0, 1.0, 1.0 }
                      });

    sources.push_back({
                              Vec3 {image_width, 500, image_width * 2 },
                              Color { 1.0, 1.0, 1.0 }
                      });

    sources.push_back({
                              Vec3 {image_width * 0.5, 0, image_width * 1.5 },
                              Color { 1.0, 1.0, 1.0 }
                      });

    sources.push_back({
                              Vec3 {0, 0, image_width * 2  },
                              Color { 1.0, 1.0, 1.0 }
                      });

    sources.push_back({
                              Vec3 {0, image_height * 1.5, image_width * 2.7  },
                              Color { 1.0, 1.0, 1.0 }
                      });
    sources.push_back({
                              Vec3 {0, image_height * 1.5, image_width * 5  },
                              Color { 1.0, 1.0, 1.0 }
                      });
    sources.push_back({
                              Vec3 {0, image_height * 1.5, image_width * 4  },
                              Color { 1.0, 1.0, 1.0 }
                      });
}

void AppGUI(Scene& scene, GLuint texture_id, int* image) {
    ImGui::SetNextWindowSize(ImVec2 {});
    ImGui::SetNextWindowPos(ImVec2 {});
    ImGui::Begin("Raytracing", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::BeginGroup();

    ImGui::PushItemWidth(-image_width);
    ImGui::InputFloat("Znear", &scene.zn);
    ImGui::InputFloat("Zoom factor", &scene.zoom_factor);
    ImGui::InputFloat("Azimuth", &scene.azimuth);
    ImGui::InputFloat("Attitude", &scene.attitude);
    ImGui::InputInt("Depth", &scene.depth);

    if (ImGui::Button("Render")) {
        const double start = omp_get_wtime();
        Raytracing(scene.camera(),
                   scene.sources,
                   scene.primitives,
                   image,
                   scene.depth,
                   scene.background,
                   scene.ambient
        );
        UpdateTexture(texture_id, image, image_width, image_height);
        const double end = omp_get_wtime();
        auto time = end - start;
        std::cout << time << '\n';
    }

    ImGui::EndGroup();
    ImGui::SameLine();
    ImGui::Image((void*)(intptr_t) texture_id, ImVec2(image_width, image_height));

    ImGui::End();
}

void MainLoop(Scene& scene, int* image, GLuint texture_id) {
    AppGUI(scene, texture_id, image);
}

GLFWwindow* InitImgui() {
    if (!glfwInit()) {
        return nullptr;
    }

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Raytracing", nullptr, nullptr);
    if (!window) {
        return nullptr;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        return nullptr;
    }
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();
    return window;
}

void FillStrangeScene(Scene& scene) {
    FillBoxScene(scene.primitives, scene.sources, Box {
                         Vec3{0, 0, 2 * image_width},
                         image_width, image_height, 3 * image_width,
                         Vec3{-1, 0, 0},
                         Vec3{0, 1, 0},
                         Vec3{0, 0, -1}
                 },
                 Material {
                         Color { 0.9, 0.9, 0.9 },
                         Color {1, 1, 1},
                         100
                 }
    );
    scene.primitives.push_back(std::make_unique<Sphere>(
            Vec3 {0, 0, 0.75 * image_width},
            image_width / 50,
            Material {
                    Color { 0.9, 0.1, 0.5 },
                    Color {0, 0, 0},
                    100
            }
    ));
    scene.sources.push_back(Light {
            Vec3 {0, 0, 0.5 * image_width},
            Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
            Vec3 {-image_width, 0, 0.5 * image_width},
            Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
            Vec3 {image_width, 0, 0.5 * image_width},
            Color {1, 1, 1}
    });
}

int main() {
    Scene scene;
    //FillScene(scene.primitives, scene.sources);
    FillBoxScene(scene.primitives, scene.sources, Box {
            Vec3{0, 0, image_width * 0.1},
            image_width, image_height, 0.1 * image_width,
            Vec3{-1, 0, 0},
            Vec3{0, 1, 0},
            Vec3{0, 0, -1}
        },
        Material {
            Color { 1, 1, 1 },
            Color {0.0, 0.0, 0.0},
            100
        }
    );
    /*scene.primitives.push_back(std::make_unique<Sphere>(
            Vec3 {0, 0,0.5 *  image_width},
            image_width * 0.25,
            Material {
                Color { 0.9, 0.1, 0.5 },
                Color {0, 0, 0},
                100
            }
            ));*/
    scene.primitives.push_back(std::make_unique<Sphere>(
            Vec3{image_width * 0.01, -image_height * 0.5 + image_width * 0.05, image_width * 0.1},
            image_width * 0.05,
            Material {
                    Color { 0.9, 0.1, 0.5 },
                    Color {},
                    100
            }
    ));
    /*scene.sources.push_back(Light {
            Vec3{0, image_height * 0.1, image_width * 0.1},
            Color {1, 1, 1}
    });*/
    scene.sources.push_back(Light {
            Vec3{0, image_height * 0.45, image_width * 0.05},
            Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
            Vec3{0, 0, image_width * 0.05},
            Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
        Vec3 {0, 0, 0.05 * image_width},
        Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
            Vec3 {0, -image_height * 0.9, 0.05 * image_width},
            Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
            Vec3 {0, image_height * 0.9, 0.05 * image_width},
            Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
            Vec3 {-image_width * 0.9, 0, 0.05 * image_width},
            Color {1, 1, 1}
    });
    scene.sources.push_back(Light {
            Vec3 {image_width * 0.9, 0, 0.05 * image_width},
            Color {1, 1, 1}
    });
    int image[image_width * image_height];
    const double start = omp_get_wtime();
    Raytracing(scene.camera(),
               scene.sources,
               scene.primitives,
               image,
               scene.depth,
               scene.background,
               scene.ambient
    );
    const double end = omp_get_wtime();
    auto time = end - start;
    std::cout << time << '\n';

    auto window = InitImgui();
    if (!window) return EXIT_FAILURE;

    GLuint texture_id = LoadSampleTexture(image, image_width, image_height);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        MainLoop(scene, image, texture_id);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return EXIT_SUCCESS;
}
