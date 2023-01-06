#include "animator.hpp"

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

void Animator::Animate(Sprite* subject,
    const std::string& field,
    const std::vector<glm::vec3>& keypoints,
    const std::vector<float> timepoints,
    void (*callback)(Sprite*)) {
  Animator::Keyframes kf;
  kf.field = field;
  kf.keypoints = keypoints;
  kf.timepoints = timepoints;
  kf.callback = callback;
  kf.time_start = glfwGetTime();
  keyframes[subject] = kf;
}

void Animator::Update(float secs) {
  const float ms = glfwGetTime();
  std::vector<Sprite*> victims;
  for (auto itr = keyframes.begin(); itr != keyframes.end(); itr++) {
    Keyframes& kfs = itr->second;
    Sprite* subject = itr->first;
    const float elapsed = ms - kfs.time_start;
    const int N = int(kfs.keypoints.size());
    while (kfs.idx+1 < N && kfs.timepoints[kfs.idx+1] < elapsed) {
      kfs.idx ++;
    }
    if (kfs.idx == N-1) {
      victims.push_back(subject);
      kfs.done = true;
      if (kfs.callback) {
        kfs.callback(subject);
      }
      continue;
    }
    float t0 = kfs.timepoints[kfs.idx], t1 = kfs.timepoints[kfs.idx+1];
    float completion = (elapsed - t0) / (t1 - t0);
    glm::vec3 value = glm::mix(kfs.keypoints[kfs.idx], kfs.keypoints[kfs.idx+1], completion);

    // TODO: Can replace with real reflection?
    if (kfs.field == "pos") {
      subject->pos = value;
    } else if (kfs.field == "scale") {
      ChunkSprite* cs = dynamic_cast<ChunkSprite*>(subject);
      if (cs) 
        cs->scale = value;
    }
  }

  for (Sprite* sp : victims) {
    keyframes.erase(sp);
  }
}