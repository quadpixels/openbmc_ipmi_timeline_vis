#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() {
  pos     = glm::vec3(0.0f, 160.0f, 32.0f);
  lookdir = glm::normalize(-pos);
  up      = glm::normalize(glm::vec3(0, 1, -5));
}

glm::mat4 Camera::GetViewMatrix() {
  return glm::lookAt(pos, pos + lookdir, up);
}