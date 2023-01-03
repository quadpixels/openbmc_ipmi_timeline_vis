#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

#include "sprite.hpp"

class Animator {
public:
  // How to do Reflection?
  struct Keyframes {
    std::string field;
    std::vector<glm::vec3> keypoints;
    std::vector<float> timepoints;  // Corresponding to timelines.
    int idx;
    bool done;
    float time_start;
    void (*callback)(Sprite* subject);  // Callback to be executed on the subject
    Keyframes() {
      idx = 0; done = false;
    }
  };
  std::unordered_map<Sprite*, Keyframes> keyframes;
  void Animate(Sprite* subject,
    const std::string& field,
    const std::vector<glm::vec3>& keyframes,
    const std::vector<float> timepoints,
    void (*callback)(Sprite*));
  void Update(float secs);
};