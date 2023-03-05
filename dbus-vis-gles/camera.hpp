#pragma once

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

class Camera {
public:
  Camera();
  glm::mat4 GetViewMatrix();
  void Update(float);
  void CrystalBall(const glm::vec3& from);
  glm::vec3 pos, lookdir, up;
};

class DirectionalLight {
public:
  glm::mat4 P, V;
  glm::vec3 dir, pos;
  DirectionalLight();
  DirectionalLight(const glm::vec3& dir, const glm::vec3& pos,
    float z_min = -100.0f, float z_max = 340.0f);
};