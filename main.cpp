#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <chrono>

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

struct Scene {
    std::vector<Sphere> spheres = {};
    std::vector<Light> sources = {};
    Camera camera {
    Vec3 { 0, image_height / 2.0, 0 },
    Vec3 { 0, image_height / 2.0, 1 },
    Vec3 { 0, 1, 0 },
    image_width * 0.5, 5 * image_width,
    image_width, image_height
    };
    Color background {0.0, 0.0, 0.0};
    Color ambient {0.01, 0.01, 0.01};
    int depth = 1;
};

void FillScene(Scene& scene) {
    auto& spheres = scene.spheres;
    spheres.push_back({
                              Vec3 { -image_width, 0, image_width * 3 },
                              image_width / 2.0f,
                              {
                                      Color { 0.1, 0.1, 0.9 },
                                      Color { 0, 0, 0 },
                                      100
                              }
                      });
    spheres.push_back({
                              Vec3 { -image_width * 0.7f, image_height * 1.5, image_width * 4 },
                              image_width / 2.0f,
                              {
                                      Color { 0.5, 0.1, 0.9 },
                                      Color { 1, 1, 1 },
                                      100
                              }
                      });
    spheres.push_back({
                              Vec3 { 0, 0, image_width * 3 },
                              image_width / 2.0f,
                              {
                                      Color { 0.658, 0.658, 0.658 },
                                      Color { 0.658, 0.658, 0.658 },
                                      150
                              }
                      });

    spheres.push_back({
                              Vec3 { image_width, 0, image_width * 2 },
                              image_width / 2.0f,
                              {
                                      Color { 1, 1, 1 },
                                      Color { 0, 0, 0 },
                                      0
                              }
                      });

    auto& sources = scene.sources;
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
}

void AppGUI(GLuint texture_id) {
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin("Demo window", nullptr, ImGuiWindowFlags_NoResize);
    if (ImGui::Button("Hello!")) {
        std::cout << "Hello!\n";
    }

    ImGui::Image((void*)(intptr_t) texture_id, ImVec2(image_width, image_height));

    ImGui::End();
}

void MainLoop(Scene& scene, int* image, GLuint texture_id) {
    /*Raytracing(scene.camera,
               scene.sources,
               scene.spheres,
               image,
               scene.depth,
               scene.background,
               scene.ambient
    );
    UpdateTexture(texture_id, image, image_width, image_height);*/
    AppGUI(texture_id);
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

int main() {
    Scene scene;
    FillScene(scene);
    int image[image_width * image_height];
    std::chrono::milliseconds start = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
    );
    Raytracing(scene.camera,
               scene.sources,
               scene.spheres,
               image,
               scene.depth,
               scene.background,
               scene.ambient
    );
    std::chrono::milliseconds end = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
    );
    auto time = end - start;
    std::cout << time.count() / 1000.0 << '\n';

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
