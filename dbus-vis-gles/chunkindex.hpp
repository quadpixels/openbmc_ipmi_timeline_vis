#pragma once

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "chunk.hpp"

// Index for multiple Chunk's
class ChunkIndex {
public:
  glm::vec3 Size();
  ChunkIndex() : x_len(0), y_len(0), z_len(0) { }
  ChunkIndex(unsigned _xlen, unsigned _ylen, unsigned _zlen);
  virtual void SetVoxel(unsigned x, unsigned y, unsigned z, int vox) = 0;
  virtual void SetVoxel(const glm::vec3& p, int vox) = 0;
  virtual void SetVoxelSphere(const glm::vec3& p, float radius, int vox) = 0;
  virtual int  GetVoxel(unsigned x, unsigned y, unsigned z) = 0;
  virtual bool IntersectPoint(const glm::vec3& p) = 0;
  virtual void Render(
      const glm::vec3& pos,
      const glm::vec3& scale,
      const glm::mat3& orientation,
      const glm::vec3& anchor) = 0;
  virtual ~ChunkIndex() {}
  virtual void Fill(int vox) = 0;
  glm::vec3 GetCentroid();
protected:
  unsigned x_len, y_len, z_len;
  virtual bool GetNeighbors(Chunk* which, Chunk* neighs[26]) = 0;
};

class ChunkGrid : public ChunkIndex {
public:
  ChunkGrid() : xdim(0), ydim(0), zdim(0) { }
  ChunkGrid(unsigned _xlen, unsigned _ylen, unsigned _zlen);
  ChunkGrid(const char* vox_fn);
  ChunkGrid(const ChunkGrid& other);
  virtual void Render(
      const glm::vec3& pos,
      const glm::vec3& scale,
      const glm::mat3& orientation,
      const glm::vec3& anchor);
  virtual void SetVoxel(unsigned x, unsigned y, unsigned z, int v);
  virtual void SetVoxel(const glm::vec3& p, int vox);
  virtual void SetVoxelSphere(const glm::vec3& p, float radius, int vox);
  virtual int  GetVoxel(unsigned x, unsigned y, unsigned z);
  virtual bool IntersectPoint(const glm::vec3& p);
  virtual void Fill(int vox);
protected:
  Chunk* GetChunk(int x, int y, int z, int* local_x, int* local_y, int* local_z);
  void Init(unsigned _xlen, unsigned _ylen, unsigned _zlen);
  unsigned xdim, ydim, zdim;
  std::vector<Chunk*> chunks;
  virtual bool GetNeighbors(Chunk* which, Chunk* neighs[26]);
  int IX(int x, int y, int z) {
    return x*ydim*zdim + y*zdim + z;
  }
  void FromIX(int ix, int& x, int& y, int& z);
};