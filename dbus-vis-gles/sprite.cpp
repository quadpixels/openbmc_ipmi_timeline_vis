#include "sprite.hpp"

void Sprite::Update(float secs) {
  pos += vel * secs;
}

void ChunkSprite::Init() {
  orientation = glm::mat3(1);
  scale = glm::vec3(1,1,1);
  // Default Anchor: centered
  anchor = chunk->Size() * 0.5f;
}

glm::vec3 ChunkSprite::GetLocalCoord(const glm::vec3& p_world) {
  glm::vec3 p_local = glm::inverse(orientation) * (p_world - pos);
  glm::vec3 pc = (p_local / scale) + anchor; // pc = point_chunk
  return pc;
}

bool ChunkSprite::IntersectPoint(const glm::vec3& p_world) {
  glm::vec3 pc = GetLocalCoord(p_world);
  return (chunk->GetVoxel(unsigned(pc.x), unsigned(pc.y), unsigned(pc.z)) > 0);
}

bool ChunkSprite::IntersectPoint(const glm::vec3& p_world, int tolerance) {
  glm::vec3 p_local = glm::inverse(orientation) * (p_world - pos);
  glm::vec3 pc = p_local / scale + anchor; // pc = point_chunk
  for (int dx=-tolerance; dx<=tolerance; dx++) {
    for (int dy=-tolerance; dy<=tolerance; dy++) {
      for (int dz=-tolerance; dz<=tolerance; dz++) {
        int xx = int(pc.x) + dx, yy = int(pc.y) + dy, zz = int(pc.z) + dz;
        if (xx >= 0 && yy >= 0 && zz >= 0) {
          if (chunk->GetVoxel(unsigned(xx), unsigned(yy), unsigned(zz)) > 0) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

void ChunkSprite::RotateAroundGlobalAxis(const glm::vec3& axis, const float deg) {
  glm::mat4 o4(orientation);
  o4 = glm::rotate(o4, deg*3.14159f/180.0f, glm::inverse(orientation)*axis);
  orientation = glm::mat3(o4);
}

void ChunkSprite::RotateAroundLocalAxis(const glm::vec3& axis, const float deg) {
  glm::mat4 o4(orientation);
  o4 = glm::rotate(o4, deg*3.14159f/180.0f, axis);
  orientation = glm::mat3(o4);
}

void ChunkSprite::Render() {
  chunk->Render(pos, scale, orientation, anchor);
}