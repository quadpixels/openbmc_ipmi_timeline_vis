
#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "util.hpp"
#include "camera.hpp"

class Scene {
public:
  virtual void Render() = 0;
  virtual void Update(float secs) = 0;
};

class HelloTriangleScene : public Scene {
public:
  unsigned int vbo_solid, vbo_line; // Buffer
  // Shader Program = VS + FS.
  unsigned int shader_program;
  HelloTriangleScene();
  void Render();
  void Update(float secs) {}
};

class PaletteScene : public Scene {
public:
  unsigned int vbo;
  unsigned int shader_program;
  PaletteScene();
  void Render();
  void Update(float secs) {}
};

class RotatingCubeScene : public Scene {
public:
  unsigned int vbo;
  unsigned int ebo;
  unsigned int shader_program;
  glm::mat4 projection_matrix;
  glm::mat4 model_matrix;
  Camera camera;
  RotatingCubeScene();
  void Render();
  void Update(float secs);
};