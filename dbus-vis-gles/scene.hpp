
#include <vector>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "camera.hpp"
#include "chunk.hpp"
#include "util.hpp"

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
  unsigned int vbo_lines;
  unsigned int ebo_lines;
  glm::mat4 projection_matrix;
  std::vector<glm::vec3> positions;
  std::vector<glm::mat3> orientations;
  Camera camera;
  RotatingCubeScene();
  void Render();
  void Update(float secs);
};

class OneChunkScene : public Scene {  // Show 1 Vox
public:
  // For backdrop
  unsigned int vbo;
  unsigned int ebo;

  unsigned int shader_program;
  glm::mat4 projection_matrix;
  Camera camera;
  DirectionalLight directional_light;
  Chunk chunk;
  OneChunkScene();
  void Render();
  void Update(float secs);
};