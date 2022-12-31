
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "util.hpp"

class Scene {
public:
  virtual void Render() = 0;
};

class HelloTriangleScene : public Scene {
public:
  unsigned int vbo_solid, vbo_line; // Buffer
  // Shader Program = VS + FS.
  unsigned int shader_program;
  HelloTriangleScene();
  void Render();
};

class PaletteScene : public Scene {
  public:
  unsigned int vbo;
  unsigned int shader_program;
  PaletteScene();
  void Render();
};