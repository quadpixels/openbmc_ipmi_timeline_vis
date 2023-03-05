
// 搞事情专用，嘿嘿嘿

#include "scene.hpp"

extern int WIN_W, WIN_H;

AccompanimentScene::AccompanimentScene() {
  chunk_assets[AssetID::Scene20230226] = new ChunkGrid("vox/sxqn.vox");
  backdrop = new OneChunkScene::Backdrop(50, 251);
  backdrop->pos = glm::vec3(0, -20, 0);

  directional_light = new DirectionalLight(glm::vec3(-1,-3,1), glm::vec3(1,3,-1)*50.0f);
  depth_fbo = new DepthOnlyFBO(WIN_W, WIN_H);
  const float ratio = WIN_W * 1.0f / WIN_H;
  const float range_y = 60.0f;
  // 用于显示的话，Y要倒过来
  //projection_matrix = glm::perspective(60.0f*3.14159f/180.0f, WIN_W*1.0f/WIN_H, 0.1f, 999.0f);
  projection_matrix = glm::ortho(-range_y * ratio, range_y * ratio, 
                                 -range_y, range_y, -100.f, 499.f);

  CreateSprite(AssetID::Scene20230226, glm::vec3(0, 0, 0));
}

void AccompanimentScene::Render() {
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glUseProgram(Chunk::shader_program);

  unsigned v_loc = glGetUniformLocation(Chunk::shader_program, "V");
  unsigned p_loc = glGetUniformLocation(Chunk::shader_program, "P");
  unsigned dir_light_loc = glGetUniformLocation(Chunk::shader_program, "dir_light");
  unsigned lightpv_loc = glGetUniformLocation(Chunk::shader_program, "lightPV");
  glm::mat4 V;
  glm::mat4 P;
  V = directional_light->V;
  P = directional_light->P;
  glUniformMatrix4fv(v_loc, 1, false, &V[0][0]);
  glUniformMatrix4fv(p_loc, 1, false, &P[0][0]);
  glUniform3f(dir_light_loc, directional_light->dir.x,
                             directional_light->dir.y,
                             directional_light->dir.z);


  // Depth pass
  depth_fbo->Bind();
  glClear(GL_DEPTH_BUFFER_BIT);
  backdrop->Render();
  for (const auto& s : sprites) {
    s->sprite->Render();
  }
  depth_fbo->Unbind();

  // Normal pass
  glUseProgram(Chunk::shader_program);
  glm::mat4 lightPV = directional_light->P * directional_light->V;
  glUniformMatrix4fv(lightpv_loc, 1, false, &lightPV[0][0]);
  V = camera.GetViewMatrix();
  P = projection_matrix;
  glUniformMatrix4fv(v_loc, 1, false, &V[0][0]);
  glUniformMatrix4fv(p_loc, 1, false, &P[0][0]);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, depth_fbo->tex);

  backdrop->Render();
  for (const auto& s : sprites) {
    s->sprite->Render();
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);
}

void AccompanimentScene::Update(float secs) {
  glm::vec3 p0 = glm::vec3(-30, 30, 30);
  camera.lookdir = glm::normalize(-p0);

  glm::vec3 local_x = glm::normalize(glm::cross(-p0, glm::vec3(0, 1, 0)));
  glm::vec3 local_y = glm::normalize(glm::cross(local_x, -p0));
  glm::vec3 local_z = glm::normalize(glm::cross(local_x, local_y));

  camera.up = local_y;
  camera.pos = p0;
}

AccompanimentScene::SpriteAndProperty* AccompanimentScene::CreateSprite(
  AccompanimentScene::AssetID asset_id, const glm::vec3& pos) {
  SpriteAndProperty* ret = new SpriteAndProperty();
  Sprite* s = new ChunkSprite(chunk_assets.at(asset_id));
  s->pos = pos;
  ret->sprite = dynamic_cast<ChunkSprite*>(s);
  sprites.push_back(ret);
  return ret;
}
