// 2023-01-01

#include "scene.hpp"

extern int WIN_W, WIN_H;

DBusPCAPScene::DBusPCAPScene() {
  backdrop = new OneChunkScene::Backdrop(256, 251);
  directional_light = new DirectionalLight(glm::vec3(-1,-3,1), glm::vec3(1,3,-1)*50.0f);
  depth_fbo = new DepthOnlyFBO(WIN_W, WIN_H);
  projection_matrix = glm::perspective(60.0f*3.14159f/180.0f, WIN_W*1.0f/WIN_H, 0.1f, 499.0f);

  chunk_assets[AssetID::OpenBMC] = new ChunkGrid("vox/openbmc.vox");
  CreateSprite(AssetID::OpenBMC, glm::vec3(0,2,0));
}

void DBusPCAPScene::Render() {
  glEnable(GL_DEPTH_TEST);
  glUseProgram(Chunk::shader_program);

  // Prepare pipeline states for the depth-only pass
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

  // Depth-only pass
  depth_fbo->Bind();
  glClear(GL_DEPTH_BUFFER_BIT);
  backdrop->Render();
  for (Sprite* s : sprites) {
    s->Render();
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
  for (Sprite* s : sprites) {
    s->Render();
  }

  glUseProgram(0);
  MyCheckError("dbus pcap scene render");
}

void DBusPCAPScene::Update(float secs) {
  
}

Sprite* DBusPCAPScene::CreateSprite(DBusPCAPScene::AssetID asset_id, const glm::vec3& pos) {
  Sprite* s = new ChunkSprite(chunk_assets.at(asset_id));
  s->pos = pos;
  sprites.push_back(s);
  return s;
}