#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "util.hpp"
#include "scene.hpp"

GLFWwindow* g_window;
double g_last_secs = 0;

int WIN_W = 1024, WIN_H = 640;

Scene* g_curr_scene;
HelloTriangleScene* g_hellotriangle;
OneChunkScene*      g_onechunk;
PaletteScene*       g_paletteview;
RotatingCubeScene*  g_rotating_cube;

void Update(float delta_secs) {
  g_curr_scene->Update(delta_secs);
}

void Render() {
  glViewport(0, 0, WIN_W, WIN_H);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(1.0f, 1.0f, 0.9f, 1.0f);
  glDisable(GL_DEPTH_TEST);

  g_curr_scene->Render();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  switch (key) {
  case GLFW_KEY_ESCAPE: {
    printf("Done.\n");
    exit(0);
  }
  case GLFW_KEY_1: g_curr_scene = g_hellotriangle; break;
  case GLFW_KEY_2: g_curr_scene = g_paletteview; break;
  case GLFW_KEY_3: g_curr_scene = g_rotating_cube; break;
  case GLFW_KEY_4: g_curr_scene = g_onechunk; break;
  default: break;
  }
}

void MyInit() {
  g_hellotriangle = new HelloTriangleScene();
  g_paletteview = new PaletteScene();
  g_rotating_cube = new RotatingCubeScene();
  g_onechunk = new OneChunkScene();

  Chunk::shader_program = BuildShaderProgram("shaders/vert_mvp_palette.vs", "shaders/hellotriangle.fs");
  MyCheckError("Build Chunk's shader program");
}

void emscriptenLoop() {
  double secs = glfwGetTime();
  glfwPollEvents();
  Update(secs - g_last_secs);
  Render();
  glfwSwapBuffers(g_window);
  g_last_secs = secs;
}

int main(int argc, char** argv) {
  if (glfwInit() == GLFW_FALSE) {
    printf("glfwInit returned false\n");
    exit(1);
  }

  MyCheckError("glfwInit");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  MyCheckError("set window hint");

  g_window = glfwCreateWindow(WIN_W, WIN_H, __FILE__, NULL, NULL);
  MyCheckError("created window");

  // https://discourse.glfw.org/t/invalid-enum-after-glfwmakecontextcurrent/170
  // Happens on ZX C960, ignoring
  glfwMakeContextCurrent(g_window);
  MyCheckError("make context current", true);

  glfwSetKeyCallback(g_window, KeyCallback);

  int major, minor, revision;
  glfwGetVersion(&major, &minor, &revision);
  printf("GLFW version: %d.%d.%d\n", major, minor, revision);
  const char* sz = (const char*)glGetString(GL_VERSION);
  printf("OpenGL Version String: %s\n", sz);
  const char* sz1 = (const char*)glGetString(GL_VENDOR);
  printf("OpenGL Vendor String: %s\n", sz1);
  const char* sz2 = (const char*)glGetString(GL_RENDERER);
  printf("OpenGL Renderer String: %s\n", sz2);
  const char* sz3 = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  printf("OpenGL Shading Language Version: %s\n", sz3);
  
  MyCheckError("print a few strings");

  MyInit();
  g_curr_scene = g_hellotriangle;

  #ifndef __EMSCRIPTEN__
  while (!glfwWindowShouldClose(g_window)) {
    emscriptenLoop();
  }
  #else
  emscripten_set_main_loop(emscriptenLoop, 0, 1);
  #endif

  printf("Done!\n");
  exit(0);
}