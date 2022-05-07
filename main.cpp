#define GLFW_INCLUDE_NONE
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <chrono>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "raytracing.h"

void TestGeometry() { // seems that it works
    const Sphere sphere {
        .center = Vec3 { 0, 0, 5 },
        .radius = 1,
        .diffuse = Color { 0.9, 0.0, 0.0 }
    };

    const Vec3 ray { 0, -0.5, 1 };
    const Vec3 start { 0, 2, 0 };
    float intersection;
    bool result = Intersection(sphere, start, ray, &intersection);
    std::cout << intersection << '\n'
              << result << '\n';
}

void error_callback(int error, const char* description) {
    fprintf(stderr, "Error: %s\n", description);
}

void DrawCanvas() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_p0 = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
    ImVec2 canvas_sz = ImGui::GetContentRegionAvail();   // Resize canvas to what's available
    if (canvas_sz.x < 50.0f) canvas_sz.x = 50.0f;
    if (canvas_sz.y < 50.0f) canvas_sz.y = 50.0f;
    ImVec2 canvas_p1 = ImVec2(canvas_p0.x + canvas_sz.x, canvas_p0.y + canvas_sz.y);
    draw_list->AddLine(canvas_p0, canvas_p1, IM_COL32(255, 50, 50, 255));
}

constexpr int image_width = 720;
constexpr int image_height = 480;
static int image[image_width * image_height];
static GLuint texture_id;

void DrawScene() {
    ImGui::Image((void*)(intptr_t)texture_id, ImVec2(image_width, image_height));
}

GLuint FillSampleImage() {
    for (size_t y = 0; y < image_height; y++) {
        for (size_t x = 0; x < image_width; x++) {
            size_t i = (image_width * y + x);
            image[i] = IM_COL32(255 * x / image_width, 255 * y / image_height, 0, 255);
        }
    }
}

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

static float delta_x = image_width / 500.0f;
static int ticks = 0;

void RenderScene() {
    std::vector<Sphere> spheres {};
    spheres.push_back({
        Vec3 { -image_width, 0, image_width * 3 },
        image_width / 2.0f,
        Color { 0.1, 0.1, 0.9 },
        Color { 0, 0, 0 }
    });
    spheres.push_back({
                              Vec3 { -image_width * 0.7f, image_height, image_width * 4 },
                              image_width / 2.0f,
                              Color { 0.5, 0.1, 0.9 },
                              Color { 1, 1, 1 }
                      });
    spheres.push_back({
                              Vec3 { image_width - ticks * delta_x, 0, image_width * 3 },
                              image_width / 2.0f,
                              Color { 0.0, 0.0, 0.0 },
                              Color { 1, 1, 1 }
                      });

    spheres.push_back({
                              Vec3 { image_width, 0, image_width * 2 },
                              image_width / 2.0f,
                              Color { 1, 1, 1 },
                              Color { 0, 0, 0 }
                      });

    std::vector<Light> sources {};
    /*sources.push_back({
        Vec3 {image_width, image_width * 2, image_width * 3},
        Color { 10000000, 10000000, 10000000 }
    });
    sources.push_back({
          Vec3 {-image_width, -image_width * 2, image_width * 3},
          Color { 10000000, 10000000, 10000000 }
    });*/

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

    const Camera camera(
            Vec3 { 0, image_height / 2.0, 0 },
            Vec3 { 0, image_height / 2.0, 1 },
            Vec3 { 0, 1, 0 },
            image_width, 5 * image_width,
            image_width, image_height
            );

    Raytracing(camera,
               sources,
               spheres,
               image,
               Color {0.5, 0.5, 0.5},
               Color {0.0001, 0.0001, 0.0001}
               );
}

void AppGUI() {
    ImGui::SetNextWindowSize(ImVec2(0, 0));
    ImGui::Begin("Demo window", nullptr, ImGuiWindowFlags_NoResize);
    if (ImGui::Button("Hello!")) {
        std::cout << "Hello!\n";
    }

    /*ticks++;
    if (ticks % 120 == 0) {
        RenderScene();
        UpdateTexture(texture_id, image, image_width, image_height);
    }*/

    DrawScene();

    ImGui::End();
}

int imgui_main() {
    FillSampleImage();
    ticks = 500;

    std::chrono::milliseconds start = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
    );
    RenderScene();
    std::chrono::milliseconds end = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
    );
    auto time = end - start;
    std::cout << time.count() / 1000.0 << '\n';

    if (!glfwInit()) {
        return EXIT_FAILURE;
    }

    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Raytracing", nullptr, nullptr);
    if (!window) {
        return EXIT_FAILURE;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cout << "Bad stuff happened\n";
        glfwDestroyWindow(window);
        return EXIT_FAILURE;
    }
    ImGui_ImplOpenGL3_Init();
    ImGui::StyleColorsDark();

    start = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
    );
    texture_id = LoadSampleTexture(image, image_width, image_height);
    end = std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::system_clock::now().time_since_epoch()
    );
    time = end - start;
    std::cout << time.count() / 1000.0 << '\n';

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        AppGUI();

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

int main() {
    //TestGeometry();
    return imgui_main();
}
