#pragma once

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Chunk {
public:
  glm::vec3 pos;  // 在ChunkIndex中的位置，便于画ChunkIndex时使用
  const static int kSize; // 32
  static unsigned shader_program;
  static bool verbose;

  Chunk();
  Chunk(Chunk& other);
  void LoadDefault();  // 测试专用
  void BuildBuffers(Chunk* neighbors[26]);
  void Render();
  void Render(const glm::mat4& M);
  void SetVoxel(unsigned x, unsigned y, unsigned z, int v);
  int  GetVoxel(unsigned x, unsigned y, unsigned z);
  void Fill(int vox);
  bool is_dirty;
  unsigned char* block;
private:
  unsigned vbo, tri_count;
  const static float kL0;
  int* light;
  inline int IX(int x, int y, int z) {
    return kSize * kSize*x + kSize*y + z;
  }
  int GetOcclusionFactor(const float x0, const float y0, const float z0,
      const int dir, Chunk* neighs[26]);
};