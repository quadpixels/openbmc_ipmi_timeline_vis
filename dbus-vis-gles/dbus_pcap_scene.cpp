// 2023-01-01

#include "scene.hpp"

extern int WIN_W, WIN_H;

DBusPCAPScene::DBusPCAPScene() {
  backdrop = new OneChunkScene::Backdrop(256, 251);
  backdrop->pos = glm::vec3(0, -10, 0);

  directional_light = new DirectionalLight(glm::vec3(-1,-3,1), glm::vec3(1,3,-1)*50.0f);
  depth_fbo = new DepthOnlyFBO(WIN_W, WIN_H);
  projection_matrix = glm::perspective(60.0f*3.14159f/180.0f, WIN_W*1.0f/WIN_H, 0.1f, 499.0f);

  chunk_assets[AssetID::OpenBMC] = new ChunkGrid("vox/openbmc.vox");
  chunk_assets[AssetID::HwMon] = new ChunkGrid("vox/hwmon.vox");
  chunk_assets[AssetID::Background] = new ChunkGrid("vox/bg.vox");
  chunk_assets[AssetID::DefaultDaemon] = new ChunkGrid("vox/defaultdaemon.vox");

  SpriteAndProperty* sp = CreateSprite(AssetID::OpenBMC, glm::vec3(0,2,0));
  sp->usage = SpriteAndProperty::Usage::Background;
  openbmc_sprite = sp->sprite;
  openbmc_sprite->pos = glm::vec3(0, 10, 0);
  openbmc_sprite->RotateAroundGlobalAxis(glm::vec3(1,0,0), 90);

  sp = CreateSprite(AssetID::Background, glm::vec3(0,2,0));
  sp->usage = SpriteAndProperty::Usage::Background;
  bg_sprite = sp->sprite;

  if (Projectile::chunk_index == nullptr) {
    ChunkIndex* ci = new ChunkGrid(3,3,3);
    ci->SetVoxelSphere(glm::vec3(1,1,1), 1, 2);
    Projectile::chunk_index = ci;
  }

  glGenBuffers(1, &lines_vbo);
  glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*6*kNumMaxLines, NULL, GL_STREAM_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  MyCheckError("Initialize DBusPCAPScene");
  lines_buf.resize(kNumMaxLines * 6);
  line_drawing_program = BuildShaderProgram("shaders/draw_lines.vs", "shaders/draw_lines.fs");

  DBusServiceFadeIn(":1.123");
  DBusServiceFadeIn(":1.124");
  DBusServiceFadeIn(":1.125");
}

void DBusPCAPScene::Render() {
  mtx.lock();
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
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

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);

  // Depth-only pass
  depth_fbo->Bind();
  glClear(GL_DEPTH_BUFFER_BIT);
  backdrop->Render();
  for (const auto& s : sprites) {
    s->sprite->Render();
  }
  for (const auto& p : projectiles) {
    if (!p->IsProjectileVisible()) continue;
    p->sprite->Render();
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
  for (const auto &s : sprites) {
    s->sprite->Render();
  }
  for (const auto& p : projectiles) {
    if (!p->IsProjectileVisible()) continue;
    p->sprite->Render();
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, 0);
  glUseProgram(0);

  // Lines pass
  glUseProgram(line_drawing_program);
  int idx = 0;
  int num_lines = 0;
  for (const auto& p : projectiles) {
    if (!p->IsTrailVisible()) continue;
    lines_buf[idx++] = p->p0.x;
    lines_buf[idx++] = p->p0.y;
    lines_buf[idx++] = p->p0.z;
    lines_buf[idx++] = p->p1.x;
    lines_buf[idx++] = p->p1.y;
    lines_buf[idx++] = p->p1.z;
    printf("%d. (%g,%g,%g) -> (%g,%g,%g)\n",
      num_lines,
      p->p0.x, p->p0.y, p->p0.z, p->p1.x, p->p1.y, p->p1.z);
    num_lines ++;
    if (num_lines >= kNumMaxLines) break;
  }
  v_loc = glGetUniformLocation(line_drawing_program, "V");
  p_loc = glGetUniformLocation(line_drawing_program, "P");
  glUniformMatrix4fv(v_loc, 1, false, &V[0][0]);
  glUniformMatrix4fv(p_loc, 1, false, &P[0][0]);
  glBindBuffer(GL_ARRAY_BUFFER, lines_vbo);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*idx, lines_buf.data());
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), NULL);
  glEnableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glDrawArrays(GL_LINES, 0, num_lines*2);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);

  MyCheckError("dbus pcap scene render");
  mtx.unlock();
}

void DBusPCAPScene::Update(float secs) {
  mtx.lock();
  openbmc_sprite->RotateAroundGlobalAxis(glm::vec3(0,1,0), secs*120);
  std::vector<Projectile*> pnext;
  for (Projectile* p : projectiles) {
    p->Update(secs);
    
    if (!p->Done()) {
      pnext.push_back(p);
      p->Update(secs);
    } else {
      delete p;
    }
  }

  std::vector<SpriteAndProperty*> spnext;
  for (SpriteAndProperty* sp : sprites) {
    if (sp->marked_for_deletion) {
      delete sp;
    } else {
      spnext.push_back(sp);
    }
  }
  sprites = spnext;

  projectiles = pnext;
  mtx.unlock();
}

DBusPCAPScene::SpriteAndProperty* DBusPCAPScene::CreateSprite(DBusPCAPScene::AssetID asset_id, const glm::vec3& pos) {
  SpriteAndProperty* ret = new SpriteAndProperty();
  Sprite* s = new ChunkSprite(chunk_assets.at(asset_id));
  s->pos = pos;
  ret->sprite = dynamic_cast<ChunkSprite*>(s);
  sprites.push_back(ret);
  return ret;
}

void DBusPCAPScene::Test1() {
  glm::vec3 p(RandRange(-50, 50), 8, RandRange(-50, 50));
  CreateSprite(AssetID::HwMon, p);
}

void DBusPCAPScene::Test2() {
  int idx0 = -1, idx1 = -1;
  
  int num_moving_sprites = 0;
  for (SpriteAndProperty* sp : sprites) {
    if (sp->usage == SpriteAndProperty::Usage::MovingSprite) num_moving_sprites++;
  }
  if (num_moving_sprites<2) return;

  idx0 = rand() % sprites.size();
  while (true) {
    idx1 = rand() % sprites.size();
    if (sprites[idx1]->usage == SpriteAndProperty::Usage::MovingSprite) {
      if (idx0 != idx1) break;
    }
  }

  SpriteAndProperty *sp0 = sprites[idx0], *sp1 = sprites[idx1];
  Projectile* proj = new Projectile(sp0->sprite, sp1->sprite);
  projectiles.push_back(proj);
}

DBusPCAPScene::Projectile::Projectile(Sprite* from, Sprite* to) {
  from_sprite = from; to_sprite = to;
  sprite = new ChunkSprite(chunk_index);

  // 如何决定怎么显示一根线：
  // - 长度要够长，这样能看得见
  // - 持续时间也要够长，不然一下子就没了

  // 所以就这样：
  // - 固定线段的长度和速度
  // - 如果长度超过了两个东西之间的距离，就截断

  glm::vec3 p0p1 = to_sprite->pos - from_sprite->pos;
  const float p0p1_len = glm::length(p0p1); // 本体
  projectile_total_lifetime = projectile_lifetime = p0p1_len / kSpeed;
  const float p0p1_len2 = p0p1_len + kLineLength; // 尾迹
  trail_lifetime = trail_total_lifetime = p0p1_len2 / kSpeed;
}

bool DBusPCAPScene::Projectile::IsProjectileVisible() {
  return projectile_lifetime > 0;
}

bool DBusPCAPScene::Projectile::IsTrailVisible() {
  return trail_lifetime > 0;
}

bool DBusPCAPScene::Projectile::Done() {
  return (projectile_lifetime <= 0 && trail_lifetime <= 0);
}

void DBusPCAPScene::Projectile::Update(float secs) {
  // 右(+x) 朝向目标
  glm::vec3 from_pos = from_sprite->pos, to_pos = to_sprite->pos;
  glm::vec3 local_x = glm::normalize(to_pos - from_pos);
  glm::vec3 y(0, 1, 0);
  glm::vec3 local_z = glm::normalize(glm::cross(y, local_x));
  glm::vec3 local_y = glm::normalize(glm::cross(local_z, local_x));
  sprite->orientation[0] = local_x;
  sprite->orientation[1] = local_y;
  sprite->orientation[2] = local_z;

  projectile_lifetime -= secs;
  if (projectile_lifetime < 0) projectile_lifetime = 0;
  trail_lifetime -= secs;
  if (trail_lifetime < 0) trail_lifetime = 0;

  glm::vec3 line_end_pos = to_pos + local_x * kLineLength;
  const float t = 1.0f - (projectile_lifetime / projectile_total_lifetime);
  sprite->pos = glm::mix(from_pos, to_pos, t);
  const float t1 = 1.0f - (trail_lifetime / trail_total_lifetime);

  // 三种情况
  // 1. 线段还没有完全从始点伸出
  // 2. 线段处于两者之间
  // 3. 线段正消失进终止点
  const float dist = glm::length(to_pos - from_pos);
  const float dist2 = glm::length(line_end_pos - from_pos);

  p0 = glm::mix(from_pos, line_end_pos, t1);

  const float traveled = glm::length(p0 - from_pos);
  if (traveled < kLineLength) {
    p1 = from_pos; // case 1
  } else {
    p1 = p0 - (local_x * kLineLength); // case 2
    if (traveled > dist) {
      p0 = to_pos; // case 3
    }
  }
}

void DBusPCAPScene::DBusServiceFadeIn(const std::string& service) {
  mtx.lock();
  if (dbus_services.find(service) == dbus_services.end()) {
    const float pad = 2, r = kSceneRadius - pad;
    glm::vec3 p(RandRange(-r, r), 0, RandRange(-r, r));
    SpriteAndProperty* s = CreateSprite(AssetID::DefaultDaemon, p);
    s->usage = SpriteAndProperty::Usage::MovingSprite;
    s->sprite->pos.y = s->sprite->chunk->GetCentroid().y + 1;
  }
  mtx.unlock();
}

void DBusPCAPScene::DBusServiceFadeOut(const std::string& service) {
  mtx.lock();
  if (dbus_services.find(service) != dbus_services.end()) {
    auto itr = dbus_services.find(service);
    itr->second->marked_for_deletion = true;
    dbus_services.erase(itr);
  }
  mtx.unlock();
}

ChunkIndex* DBusPCAPScene::Projectile::chunk_index = nullptr;