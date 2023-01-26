// 2023-01-01

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>

#include "animator.hpp"
#include "scene.hpp"

extern int WIN_W, WIN_H;

DBusPCAPScene::DBusPCAPScene() {
  backdrop = new OneChunkScene::Backdrop(800, 251);
  backdrop->pos = glm::vec3(0, -10, 0);

  directional_light = new DirectionalLight(glm::vec3(-1,-3,1), glm::vec3(1,3,-1)*50.0f);
  depth_fbo = new DepthOnlyFBO(WIN_W, WIN_H);
  projection_matrix = glm::perspective(60.0f*3.14159f/180.0f, WIN_W*1.0f/WIN_H, 0.1f, 999.0f);

  chunk_assets[AssetID::OpenBMC] = new ChunkGrid("vox/openbmc.vox");
  chunk_assets[AssetID::HwMon] = new ChunkGrid("vox/hwmon.vox");
  chunk_assets[AssetID::Background] = new ChunkGrid("vox/bg.vox");
  chunk_assets[AssetID::DefaultDaemon] = new ChunkGrid("vox/defaultdaemon.vox");
  chunk_assets[AssetID::ObjectMapper] = new ChunkGrid("vox/mapper.vox");
  chunk_assets[AssetID::IpmiHost] = new ChunkGrid("vox/ipmi.vox");
  chunk_assets[AssetID::EntityManager] = new ChunkGrid("vox/em.vox");

  SpriteAndProperty* sp = CreateSprite(AssetID::OpenBMC, glm::vec3(0,2,0));
  sp->usage = SpriteAndProperty::Usage::Background;
  openbmc_sprite = sp->sprite;
  openbmc_sprite->pos = glm::vec3(0, 10, 0);
  openbmc_sprite->RotateAroundGlobalAxis(glm::vec3(1,0,0), 90);

  sp = CreateSprite(AssetID::Background, glm::vec3(0,2,0));
  sp->usage = SpriteAndProperty::Usage::Background;
  bg_sprite = sp->sprite;
  bg_sprite->scale = glm::vec3(4,1,4);

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

  cam_zoom = new CamZoom(camera);
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
    num_lines ++;
    if (num_lines >= kNumMaxLines) break;
  }

  // Signal waves
  for (SignalWave* sw : signal_waves) {
    if (sw->Done()) continue;
    if (num_lines + SignalWave::kBreaks) {
      std::vector<float> vertexes = sw->GenerateVertexList();
      for (float x : vertexes) {
        lines_buf[idx++] = x;
      }
      num_lines += SignalWave::kBreaks;
    }
  }

  // Arena boundary
  if (num_lines + 4 < kNumMaxLines) {
    const float arena_coords[] = {
      -kSceneRadius, 0, -kSceneRadius,
      -kSceneRadius, 0,  kSceneRadius,
       kSceneRadius, 0,  kSceneRadius,
       kSceneRadius, 0, -kSceneRadius,
    };
    for (int i=0; i<4; i++) {
      const float x0 = arena_coords[i*3];
      const float y0 = arena_coords[i*3 + 1];
      const float z0 = arena_coords[i*3 + 2];
      const float x1 = arena_coords[(i*3+3) % 12];
      const float y1 = arena_coords[(i*3+4) % 12];
      const float z1 = arena_coords[(i*3+5) % 12];
      lines_buf[idx++] = x0;
      lines_buf[idx++] = y0;
      lines_buf[idx++] = z0;
      lines_buf[idx++] = x1;
      lines_buf[idx++] = y1;
      lines_buf[idx++] = z1;
      num_lines ++;
    }
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

  animator.Update(secs);
  cam_zoom->Update(secs);
  camera.pos = glm::mix(camera.pos, cam_zoom->GetCamTargetPosition(),
    pow(0.95,  secs/0.016));

  std::vector<Projectile*> pnext;
  for (Projectile* p : projectiles) {
    p->Update(secs);
    if (!p->Done()) {
      pnext.push_back(p);
    } else {
      delete p;
    }
  }
  projectiles = pnext;

  std::vector<SignalWave*> swnext;
  for (SignalWave* sw : signal_waves) {
    sw->Update(secs);
    if (!sw->Done()) {
      swnext.push_back(sw);
    } else {
      delete sw;
    }
  }
  signal_waves = swnext;

  std::vector<SpriteAndProperty*> spnext;
  for (SpriteAndProperty* sp : sprites) {
    if (sp->marked_for_deletion) {
      delete sp;
    } else {
      spnext.push_back(sp);
    }
  }
  sprites = spnext;

  const int NS = int(sprites.size());
  for (int i=0; i<NS; i++) {
    SpriteAndProperty* sp = sprites[i];
    if (sp->usage != SpriteAndProperty::Usage::MovingSprite) continue;
    for (int j=i+1; j<NS; j++) {
      SpriteAndProperty* sp1 = sprites[j];
      if (sp1->usage != SpriteAndProperty::Usage::MovingSprite) continue;
      glm::vec3 p0p1 = sp1->sprite->pos - sp->sprite->pos;
      float l = glm::length(p0p1);
      if (l < kRepulsionDistThresh) {
        glm::vec3 delta_v = glm::normalize(p0p1) * kRepulsionFactor * (kRepulsionDistThresh - l) * secs;
        delta_v.y = 0; // Y位置始终不动
        sp1->sprite->vel += delta_v;
        sp->sprite->vel -= delta_v;
      }
    }

    // 越界的情况的处理
    glm::vec3& p = sp->sprite->pos;
    glm::vec3& v = sp->sprite->vel;
    const float xlimit = kSceneRadius - sp->sprite->chunk->GetCentroid().x;
    const float zlimit = kSceneRadius - sp->sprite->chunk->GetCentroid().z;
    if (p.x > xlimit) {
      p.x = xlimit;
      if (v.x > 0) v.x = -v.x;
    } else if (p.x < -xlimit) {
      p.x = -xlimit;
      if (v.x < 0) v.x = -v.x;
    }

    if (p.z > zlimit) {
      p.z = zlimit;
      if (v.z > 0) v.z = -v.z;
    } else if (p.z < -zlimit) {
      p.z = -zlimit;
      if (v.z < 0) v.z = -v.z;
    }
  }

  float damp = pow(kDampening, secs / 0.016f);
  for (SpriteAndProperty* sp : spnext) {
    sp->sprite->pos += sp->sprite->vel * secs;
    sp->sprite->vel *= damp;
  }

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
  SpriteAndProperty* sp = CreateSprite(AssetID::HwMon, p);
  sp->usage = SpriteAndProperty::Usage::MovingSprite;
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



DBusPCAPScene::SpriteAndProperty* DBusPCAPScene::DBusServiceFadeIn(const std::string& service,
  const std::string& path, const std::string& iface, const std::string& member, bool is_sig) {
  mtx.lock();
  SpriteAndProperty* ret;
  if (dbus_services.find(service) == dbus_services.end()) {
    printf("%s fade in\n", service.c_str());
    const float pad = 2, r = kSceneRadius - pad;
    glm::vec3 p(RandRange(-r, r), 0, RandRange(-r, r));

    AssetID asset_id = AssetID::DefaultDaemon;

    // Guess what asset we should use
    if (service == "xyz.openbmc_project.ObjectMapper") {
      asset_id = AssetID::ObjectMapper;
    } else if (path.find("/xyz/openbmc_project/sensors/") == 0 && is_sig) {
      asset_id = AssetID::HwMon;
    } else if (path == "/xyz/openbmc_project/Ipmi" && member == "execute") {
      asset_id = AssetID::IpmiHost;
    }

    SpriteAndProperty* s = CreateSprite(asset_id, p);
    s->usage = SpriteAndProperty::Usage::MovingSprite;
    s->sprite->pos.y = s->sprite->chunk->GetCentroid().y + 1;

    // 特例：把mapper缩小一点
    if (asset_id == AssetID::ObjectMapper) {
      s->sprite->scale *= 0.75;
    }

    dbus_services[service] = s;
    ret = s;

    std::vector<glm::vec3> kps = {
      s->sprite->pos + glm::vec3(0, 100, 0),
      s->sprite->pos
    };
    std::vector<float> tps = {
      0, 0.6f // seconds
    };
    animator.Animate(s->sprite, "pos", kps, tps, nullptr);
  }
  mtx.unlock();
  return ret;
}

static std::unordered_map<Sprite*, DBusPCAPScene::SpriteAndProperty*> s2sp;

void DBusPCAPScene::DBusServiceFadeOut(const std::string& service) {
  mtx.lock();
  if (dbus_services.find(service) != dbus_services.end()) {
    auto itr = dbus_services.find(service);
    s2sp[itr->second->sprite] = itr->second;
    animator.Animate(itr->second->sprite, "scale", {
      itr->second->sprite->scale, glm::vec3(0,0,0)
    }, { 0, 1 }, [](Sprite* s){
      s2sp[s]->marked_for_deletion = true;
      s2sp.erase(s);
    });
    dbus_services.erase(itr);
  }
  mtx.unlock();
}

DBusPCAPScene::SpriteAndProperty* DBusPCAPScene::GetOrCreateDBusServiceSprite(const std::string& service,
  const std::string& path, const std::string& iface, const std::string& member, bool is_sig) {
  SpriteAndProperty* ret;
  if (dbus_services.find(service) != dbus_services.end()) {
    return dbus_services.at(service);
  } else {
    return DBusServiceFadeIn(service, path, iface, member, is_sig);
  }
}

void DBusPCAPScene::DBusMakeMethodCall(const std::string& from, const std::string& to,
  const std::string& path, const std::string& interface, const std::string& member) {
  DBusPCAPScene::SpriteAndProperty *sp_from = GetOrCreateDBusServiceSprite(from,
    path, interface, member, false);
  DBusPCAPScene::SpriteAndProperty *sp_to = GetOrCreateDBusServiceSprite(to,
    path, interface, member, false);
  mtx.lock();
  Projectile* proj = new Projectile(sp_from->sprite, sp_to->sprite);
  projectiles.push_back(proj);
  cam_zoom->Push();
  mtx.unlock();
}

DBusPCAPScene::SignalWave::SignalWave(Sprite* from) {
  pos = from->pos;
  lifetime = kDefaultTotalLifetime;
  total_lifetime = kDefaultTotalLifetime;
}

bool DBusPCAPScene::SignalWave::Done() {
  return lifetime <= 0;
}

void DBusPCAPScene::SignalWave::Update(float secs) {
  lifetime -= secs;
  if (lifetime < 0) {
    lifetime = 0;
  }
}

std::vector<float> DBusPCAPScene::SignalWave::GenerateVertexList() {
  std::vector<float> ret;
  const float t = 1.0f - (lifetime / total_lifetime);
  const float r = kRadius * t;
  for(int i=0; i<kBreaks; i++) {
    const float theta1 = Lerp(0, 2*3.141592f, i*1.0f/kBreaks);
    glm::vec3 offset1(r*cos(theta1), 0, r*sin(theta1));
    const float theta2 = Lerp(0, 2*3.141592f, (1+i)*1.0f/kBreaks);
    glm::vec3 offset2(r*cos(theta2), 0, r*sin(theta2));

    glm::vec3 vert1 = pos + offset1;
    glm::vec3 vert2 = pos + offset2;

    ret.push_back(vert1.x);
    ret.push_back(vert1.y);
    ret.push_back(vert1.z);
    ret.push_back(vert2.x);
    ret.push_back(vert2.y);
    ret.push_back(vert2.z);
  }
  return ret;
}

void DBusPCAPScene::DBusEmitSignal(const std::string& from, const std::string& path, const std::string& iface, const std::string& member) {
  DBusPCAPScene::SpriteAndProperty* sp = GetOrCreateDBusServiceSprite(
    from, path, iface, member, true);
  mtx.lock();
  SignalWave* sw = new SignalWave(sp->sprite);
  signal_waves.push_back(sw);
  cam_zoom->Push();
  mtx.unlock();
}

ChunkIndex* DBusPCAPScene::Projectile::chunk_index = nullptr;

DBusPCAPScene::CamZoom::CamZoom(const Camera& camera) {
  camera_lookdir = camera.lookdir;
  camera_focus = glm::vec3(0, 0, 0);
}

void DBusPCAPScene::CamZoom::Update(float secs) {
  speed += acc * secs;
  if (speed > speed_max) {
    speed = speed_max;
  } else if (speed < speed_min) {
    speed = speed_min;
  }
  dist += speed * secs;
  if (dist > max_dist) {
    dist = max_dist;
  } else if (dist < min_dist) {
    dist = min_dist;
  }
}

glm::vec3 DBusPCAPScene::CamZoom::GetCamTargetPosition() {
  return camera_focus - camera_lookdir*dist;
}

void DBusPCAPScene::CamZoom::Push() {
  if (speed > 0) speed = 0;
  speed -= 20.0f;
}