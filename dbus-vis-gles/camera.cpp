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

void Camera::CrystalBall(const glm::vec3& from) {
  lookdir = glm::normalize(-from);
  glm::vec3 local_x = glm::normalize(glm::cross(-from, glm::vec3(0, 1, 0)));
  glm::vec3 local_y = glm::normalize(glm::cross(local_x, -from));
  glm::vec3 local_z = glm::normalize(glm::cross(local_x, local_y));

  up = local_y;
  pos = from;
}

DirectionalLight::DirectionalLight() {
  dir = glm::normalize(glm::vec3(-1, -1, -1));
  pos = glm::vec3(0);
  P = glm::ortho(-200.f, 200.f, -200.f, 200.f, -100.f, 499.f);
  V = glm::lookAt(pos, pos + dir,
      glm::normalize(
          glm::cross(dir, glm::cross(glm::vec3(0.f,1.f,0.f), dir))));
}

DirectionalLight::DirectionalLight(const glm::vec3& _dir, const glm::vec3& _pos,
  float z_min, float z_max) {
  dir = glm::normalize(_dir); pos = _pos;
  P = glm::ortho(-400.f, 400.f, -400.f, 400.f, z_min, z_max);
  V = glm::lookAt(pos, pos + dir,
      glm::normalize(
          glm::cross(dir, glm::cross(glm::vec3(0.f,1.f,0.f), dir))));
}