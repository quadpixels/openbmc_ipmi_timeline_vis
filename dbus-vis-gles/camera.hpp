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
  glm::vec3 pos, lookdir, up;
};