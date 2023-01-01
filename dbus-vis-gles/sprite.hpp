#pragma once

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "chunkindex.hpp"
#include <map>

class Sprite {
public:
  glm::vec3 pos, vel;
  glm::mat3 orientation;
  virtual void Render() = 0;
  virtual void Update(float);
  virtual bool IntersectPoint(const glm::vec3& p_world) = 0;
  virtual bool IntersectPoint(const glm::vec3& p_world, int tolerance) = 0;
  virtual ~Sprite() { }
};

class ChunkSprite : public Sprite {
public:
  ChunkSprite(ChunkIndex* _c) : chunk(_c) { Init(); }
  void Init();
  glm::vec3 GetLocalCoord(const glm::vec3& p_world);
  virtual bool IntersectPoint(const glm::vec3& p_world);
  virtual bool IntersectPoint(const glm::vec3& p_world, int tolerance);
  ChunkIndex* chunk;
  glm::vec3 scale, anchor;
  void RotateAroundLocalAxis(const glm::vec3& axis, const float deg);
  void RotateAroundGlobalAxis(const glm::vec3& axis, const float deg);
  virtual void Render();

  // Test functions
  void Test1(); // spawn a random sprite
};