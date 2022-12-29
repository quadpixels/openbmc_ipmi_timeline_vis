#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

GLFWwindow* g_window;

const int WIN_W = 1024, WIN_H = 640;

int main(int argc, char** argv) {
  if (glfwInit() == GLFW_FALSE) {
    printf("glfwInit returned false\n");
    exit(1);
  }
  
  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
  g_window = glfwCreateWindow(WIN_W, WIN_H, __FILE__, NULL, NULL);
  glfwMakeContextCurrent(g_window);
  
  const char* sz = (const char*)glGetString(GL_VERSION);
  printf("OpenGL Version String: %s\n", sz);
  const char* sz1 = (const char*)glGetString(GL_VENDOR);
  printf("OpenGL Vendor String: %s\n", sz1);
  const char* sz2 = (const char*)glGetString(GL_RENDERER);
  printf("OpenGL Renderer String: %s\n", sz2);
  const char* sz3 = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  printf("OpenGL Shading Language Version: %s\n", sz3);
  
  printf("Done!\n");
  exit(0);
}