cmake_minimum_required(VERSION 3.21)
project(untitled)

set(CMAKE_CXX_STANDARD 17)

include(FindPkgConfig)
pkg_search_module(GL REQUIRED gl)
pkg_search_module(GLEW REQUIRED glew)
pkg_search_module(GLFW REQUIRED glfw3)

set(INCLUDE_DIR ./includes)
set(OPENSOURCE_DIR /home/numi/documents/open_source)
set(IMGUI_DIR ${OPENSOURCE_DIR}/imgui)
set(IMGUI_BACK_DIR ${IMGUI_DIR}/backends)
set(STB_DIR ${OPENSOURCE_DIR}/stb)

add_executable(untitled main.cpp
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_BACK_DIR}/imgui_impl_opengl3.cpp
        ${IMGUI_BACK_DIR}/imgui_impl_glfw.cpp
        raytracing/raytracing.cpp)

target_include_directories(untitled PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_BACK_DIR}
        ${STB_DIR}
        ${INCLUDE_DIR}
        ${INCLUDE_DIR}/raytracing
        )

target_include_directories(untitled SYSTEM PUBLIC ${GL_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${GLFW_INCLUDE_DIRS})
target_link_libraries(untitled ${GL_LIBRARIES} ${GLEW_LIBRARIES} ${GLFW_LIBRARIES} ${CMAKE_DL_LIBS})
