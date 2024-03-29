// 2022-12-30

#include <vector>

#include "rendertarget.hpp"
#include "scene.hpp"
#include "sprite.hpp"
#include "util.hpp"
#include <glm/gtc/matrix_transform.hpp>

extern int WIN_W, WIN_H;
extern HelloTriangleScene* g_hellotriangle;

HelloTriangleScene::HelloTriangleScene() {
  const float R0 = 1.0f, G0 = 0.4f, B0 = 0.4f;
  const float R1 = 0.4f, G1 = 1.0f, B1 = 0.4f;
  const float R2 = 0.4f, G2 = 0.4f, B2 = 1.0f;
  // 1. Buffer
  float vertices0[] = {
    -0.9f, -0.5f, 0.0f, R0, G0, B0,
    -0.1f, -0.5f, 0.0f, R1, G1, B1,
    -0.5f,  0.5f, 0.0f, R2, G2, B2,
  };

  glGenBuffers(1, &vbo_solid);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_solid);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices0), vertices0, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  float vertices1[] = {
    0.9f, -0.5f, 0.0f, R0, G0, B0,
    0.1f, -0.5f, 0.0f, R1, G1, B1,
    0.1f, -0.5f, 0.0f, R1, G1, B1,
    0.5f,  0.5f, 0.0f, R2, G2, B2,
    0.5f,  0.5f, 0.0f, R2, G2, B2,
    0.9f, -0.5f, 0.0f, R0, G0, B0,
  };
  glGenBuffers(1, &vbo_line);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_line);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices1), vertices1, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  MyCheckError("HelloTriangleScene init\n");

  // 2. Vertex Attributes: not available for GLES 2.0

  // 3. Shader
  shader_program = BuildShaderProgram("shaders/hellotriangle.vs", "shaders/hellotriangle.fs");
}

void HelloTriangleScene::Render() {
  glUseProgram(shader_program);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_solid);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), NULL); // VS中的第一个 in
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float))); // VS中的第二个 in
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glDrawArrays(GL_TRIANGLES, 0, 3);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_line);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), NULL);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  // Not supported in WebGL, b/c it's deprecated in newer versions of OpenGL
  // https://bugzilla.mozilla.org/show_bug.cgi?id=1560771
  float orig_line_width;
  glGetFloatv(GL_LINE_WIDTH, &orig_line_width);
  glLineWidth(2);
  glDrawArrays(GL_LINES, 0, 6);
  glLineWidth(orig_line_width);
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glUseProgram(0);
  MyCheckError("HelloTriangleScene::Render");
}

PaletteScene::PaletteScene() {
  const float X0 = -0.9f, X1 = 0.9f, Y0 = 0.9f, Y1 = -0.9f;
  std::vector<float> vertices;
  const int W = 16; // 256 colors
  int idx = 0;
  for (int col=0; col<W; col++) {
    for (int row=0; row<W; row++) {
      idx ++;
      const float x0 = Lerp(X0, X1, row    *1.0f/W);
      const float x1 = Lerp(X0, X1, (row+1)*1.0f/W);
      const float y0 = Lerp(Y0, Y1, col    *1.0f/W);
      const float y1 = Lerp(Y0, Y1, (col+1)*1.0f/W);

      const float data[] = {
        x0, y0, float(idx), x0, y1, float(idx), x1, y0, float(idx),
        x1, y0, float(idx), x0, y1, float(idx), x1, y1, float(idx),
      };

      for (int i=0; i<sizeof(data)/sizeof(data[0]); i++) {
        vertices.push_back(data[i]);
      }
    }
  }
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  shader_program = BuildShaderProgram("shaders/palettescene.vs", "shaders/hellotriangle.fs");
}

void PaletteScene::Render() {
  glUseProgram(shader_program);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 3*sizeof(float), NULL);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)(2*sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glDrawArrays(GL_TRIANGLES, 0, 256*6);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glUseProgram(0);
  MyCheckError("Render");
}

RotatingCubeScene::RotatingCubeScene() {
  const float L = 10.0f;
  const float verts[] = {
    // 下层
    -L, -L, -L, 10,
    -L, -L,  L, 20,
     L, -L,  L, 30,
     L, -L, -L, 40,

    // 上层
    -L, L, -L, 50,
    -L, L,  L, 60,
     L, L,  L, 70,
     L, L, -L, 80,
  };
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);


  const int idxes[] = {
    // 下层
    0,3,1, 3,2,1,
    // 四周
    4,0,5, 5,0,1, 6,5,1, 1,2,6, 6,2,3, 7,6,3, 0,4,7, 7,3,0,
    // 上层
    7,4,5, 7,5,6
  };
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxes), idxes, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  shader_program = BuildShaderProgram("shaders/vert_mvp_palette.vs", "shaders/hellotriangle.fs");

  projection_matrix = glm::perspective(60.0f*3.14159f/180.0f, WIN_W*1.0f/WIN_H, 0.1f, 499.0f);
  const int N = 5;
  positions.resize(N);
  orientations.resize(N);
  for (int i=0; i<N; i++) {
    orientations[i] = glm::mat3(1);
  }

  vbo_lines = 0;
  ebo_lines = 0;

  MyCheckError("RotatingCubeScene init");
}

void RotatingCubeScene::Update(float secs) {
  const float R = 32;
  const int N = int(positions.size());
  for (int i=0; i<int(positions.size()); i++) {
    const float theta = Lerp(0, 2 * 3.14159f, i*1.0f / N) + glfwGetTime() * 3.14159f / 3;
    glm::vec3& p = positions[i];
    p.x = R * cos(theta);
    p.z = R * sin(theta);
    glm::mat3& m = orientations[i];
    m = RotateAroundGlobalAxis(m, glm::normalize(glm::vec3(0,1,1)), secs*60);
    m = RotateAroundLocalAxis(m, glm::vec3(1,0,0), secs*60);
  }

  std::vector<float> data;
  for (int i=0; i<N; i++) {
    glm::vec3& p = positions[i];
    data.push_back(p.x);
    data.push_back(p.y);
    data.push_back(p.z);
    data.push_back(120); // color idx.
  }
  if (vbo_lines == 0) {
    glGenBuffers(1, &vbo_lines);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*4*N, NULL, GL_STATIC_DRAW);
    MyCheckError("Create vbo_lines");
  }
  glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)*4*N, data.data());
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  MyCheckError("Update vbo_lines");

  std::vector<int> data2;
  for (int i=0; i<N; i++) {
    data2.push_back(i);
    data2.push_back((i+1) % N);
  }
  if (ebo_lines == 0) {
    glGenBuffers(1, &ebo_lines);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_lines);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int)*N*2, NULL, GL_DYNAMIC_DRAW);
  }
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_lines);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, sizeof(int)*2*N, data2.data());
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  MyCheckError("Update ebo_lines");
}

void RotatingCubeScene::Render() {
  glEnable(GL_DEPTH_TEST);
  glUseProgram(shader_program);
  
  unsigned m_loc = glGetUniformLocation(shader_program, "M");
  unsigned v_loc = glGetUniformLocation(shader_program, "V");
  unsigned p_loc = glGetUniformLocation(shader_program, "P");

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(float), NULL);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(3*sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

  for (int i=0; i<int(positions.size()); i++) {
    glm::mat4 M = orientations[i];
    M = glm::translate(M, glm::inverse(orientations[i])*positions[i]);
    glm::mat4 V = camera.GetViewMatrix();
    glm::mat4 P = projection_matrix;
    glUniformMatrix4fv(m_loc, 1, false, &M[0][0]);
    glUniformMatrix4fv(v_loc, 1, false, &V[0][0]);
    glUniformMatrix4fv(p_loc, 1, false, &P[0][0]);
    
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo_lines);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4*sizeof(float), NULL);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(3*sizeof(float)));
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_lines);
  glm::mat4 M(1);
  glm::mat4 V = camera.GetViewMatrix();
  glm::mat4 P = projection_matrix;
  glUniformMatrix4fv(m_loc, 1, false, &M[0][0]);
  glUniformMatrix4fv(v_loc, 1, false, &V[0][0]);
  glUniformMatrix4fv(p_loc, 1, false, &P[0][0]);
  MyCheckError("Before rendering cubes");
  glDrawElements(GL_LINES, positions.size()*2, GL_UNSIGNED_INT, 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glUseProgram(0);
  MyCheckError("RotatingCubeScene Render");
}

OneChunkScene::Backdrop::Backdrop(float half_width, int cidx) {
  pos = glm::vec3(0);
  const float L = half_width;
  const float verts[] = {
    -L, 0, -L, 4, float(cidx), 0,
    -L, 0,  L, 4, float(cidx), 0,
     L, 0,  L, 4, float(cidx), 0,
     L, 0, -L, 4, float(cidx), 0,
  };
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  const int idxes[] = { 3,0,1, 1,2,3 };
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxes), idxes, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void OneChunkScene::Backdrop::Render() {
  unsigned m_loc = glGetUniformLocation(Chunk::shader_program, "M");
  glm::mat4 M(1);
  M = glm::translate(M, pos);
  glUniformMatrix4fv(m_loc, 1, false, &M[0][0]);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  const size_t stride = sizeof(float)*6;
  // XYZ pos
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
  glEnableVertexAttribArray(0);
  // Normal idx + Data + AO Index
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3*sizeof(GLfloat)));
  glEnableVertexAttribArray(1);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  MyCheckError("Backdrop render");
}

OneChunkScene::OneChunkScene() {
  backdrop = new Backdrop(50, 251);
  backdrop->pos = glm::vec3(0, -20, 0);
  // Chunk
  //chunk.LoadDefault();

  for (int y=15; y<=18; y++) {
    for (int x=15; x<=18; x++) {
      chunk.SetVoxel(x, y, 29, 120);
    }
  }
  chunk.SetVoxel(16, 16, 30, 120);

  //chunk.SetVoxel(1, 1, 1, 120);
  //chunk.SetVoxel(2, 1, 1, 120);
  //chunk.SetVoxel(3, 1, 1, 120);
  Chunk* null_neighs[26] = {};
  Chunk::verbose = true;
  chunk.BuildBuffers(null_neighs);
  Chunk::verbose = false;
  MyCheckError("Chunk Build Default");
  chunk.pos = glm::vec3(0,//-Chunk::kSize/2,
                        -Chunk::kSize/2,
                        -Chunk::kSize/2);

  projection_matrix = glm::perspective(60.0f*3.14159f/180.0f, WIN_W*1.0f/WIN_H, 0.1f, 499.0f);

  // Dummy (all 1.0) depth buffer
  glGenTextures(1, &depth_buffer_tex);
  glBindTexture(GL_TEXTURE_2D, depth_buffer_tex);
  const int W = 512;
  unsigned short *data2 = new unsigned short[W*W];
  for (int i=0; i<W*W; i++) {
    data2[i] = 65535;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, W, W, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, data2);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  depth_fbo = new DepthOnlyFBO(WIN_W, WIN_H);
  directional_light = new DirectionalLight(glm::vec3(-1,-3,1), glm::vec3(1,3,-1)*50.0f);
  chunkindex = new ChunkGrid(64, 64, 64);

  const int xxyy[] = { 31, 27 };
  for (int i=0; i<2; i++) {
    const int xy0 = xxyy[i];
    for (int x=xy0-1; x<xy0+2; x++) {
      for (int y=xy0-1; y<xy0+2; y++) {
        for (int z=xy0-1; z<xy0+2; z++) {
          chunkindex->SetVoxel(glm::vec3(x, y, z), 120);
        }
      }
    }
    chunkindex->SetVoxel(glm::vec3(xy0-2, xy0, xy0), 120);
    chunkindex->SetVoxel(glm::vec3(xy0+2, xy0, xy0), 120);
    chunkindex->SetVoxel(glm::vec3(xy0, xy0-2, xy0), 120);
    chunkindex->SetVoxel(glm::vec3(xy0, xy0+2, xy0), 120);
    chunkindex->SetVoxel(glm::vec3(xy0, xy0, xy0-2), 120);
    chunkindex->SetVoxel(glm::vec3(xy0, xy0, xy0+2), 120);
  }

  chunksprite = new ChunkSprite(chunkindex);
  chunksprite->pos = glm::vec3(0, 0, -10);

  camera.CrystalBall(glm::vec3(0,10,30));
}

void OneChunkScene::Render() {
  glEnable(GL_DEPTH_TEST);
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
  MyCheckError("Clearing Depth Buffer Bit");
  chunk.Render();
  //chunkindex->Render(glm::vec3(-10, 0, 10), glm::vec3(1,1,1), glm::mat3(1), chunkindex->GetCentroid());
  //chunksprite->Render();
  depth_fbo->Unbind();
  MyCheckError("Render to Depth FBO");

  // Normal pass
  glUseProgram(Chunk::shader_program);
  glm::mat4 lightPV = directional_light->P * directional_light->V;
  glUniformMatrix4fv(lightpv_loc, 1, false, &lightPV[0][0]);
  V = camera.GetViewMatrix();
  P = projection_matrix;
  glUniformMatrix4fv(v_loc, 1, false, &V[0][0]);
  glUniformMatrix4fv(p_loc, 1, false, &P[0][0]);

  glActiveTexture(GL_TEXTURE0);
  //glBindTexture(GL_TEXTURE_2D, depth_fbo->tex);
  backdrop->Render();
  //chunkindex->Render(glm::vec3(-10, 0, 10), glm::vec3(1,1,1), glm::mat3(1), chunkindex->GetCentroid());
  chunk.Render();
  chunksprite->Render();
  glBindTexture(GL_TEXTURE_2D, 0);  // Fix "illegal feedback" error detected when using WebGL
  MyCheckError("Chunk render");
}

void OneChunkScene::Update(float secs) {
  chunksprite->RotateAroundGlobalAxis(glm::vec3(0,1,0), 60*secs);
}

TextureScene::TextureScene() {
  // 1. Shader, VBO, EBO
  shader_program = BuildShaderProgram("shaders/simple_texture.vs", "shaders/simple_texture.fs");
  MyCheckError("build TextureScene shaders");
  shader_program_stencil = BuildShaderProgram("shaders/simple_texture.vs", "shaders/simple_texture_stencil.fs");
  MyCheckError("build TextureScene shaders_stencil");

  float verts[] = {
    -1,  1, 0, 0, 1,
    -1, -1, 0, 0, 0,
     1, -1, 0, 1, 0,
     1,  1, 0, 1, 1,
  };
  glGenBuffers(1, &vbo_tex);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  unsigned int idxes[] = {
    0,1,3,  3,1,2
  };
  glGenBuffers(1, &ebo_tex);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_tex);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxes), idxes, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // 2.1. a color Texture
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  unsigned char* data = new unsigned char[4*WIN_W*WIN_H];
  int idx = 0;
  for (int y=0; y<WIN_H; y++) {
    for (int x=0; x<WIN_W; x++) {
      const unsigned char r = (unsigned char)(Lerp(0, 255, x*1.0f/(WIN_W-1)));
      const unsigned char g = (unsigned char)(Lerp(0, 255, y*1.0f/(WIN_H-1)));
      data[idx++] = r;
      data[idx++] = g;
      data[idx++] = 0;
      data[idx++] = 255;
    }
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIN_W, WIN_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
  delete[] data;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glBindTexture(GL_TEXTURE_2D, 0);
  MyCheckError("Generating texture for TextureScene.");

  // 2.2. a Depth texture
  // Dummy (all 1.0) depth buffer
  glGenTextures(1, &depth_buffer_tex);
  glBindTexture(GL_TEXTURE_2D, depth_buffer_tex);
  const int W = 512;
  unsigned short *data2 = new unsigned short[W*W];
  for (int i=0; i<W*W; i++) {
    data2[i] = i % 65535;
  }
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, W, W, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, data2);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, 0);

  // 3. FBO to draw to the screen.
  fbo_mode = 2;
  basic_fbo = new BasicFBO(WIN_W, WIN_H);
  depth_fbo = new DepthOnlyFBO(WIN_W, WIN_H);
  depth_fbo2 = new DepthOnlyFBO2(WIN_W, WIN_H);
  depth_stencil_fbo = new DepthStencilFBO(WIN_W, WIN_H);
}

void TextureScene::Render() {

  {
    basic_fbo->Bind();
    g_hellotriangle->Render();
    basic_fbo->Unbind();
  }

  {
    switch (fbo_mode) {
      case 0: depth_fbo->Bind(); break;
      case 1: depth_fbo2->Bind(); break;
      case 2: depth_stencil_fbo->Bind(); break;
    }
    MyCheckError("Bind FBO");

    glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    MyCheckError("Clear FBO 1");
    glClearDepthf(1.0f);
    MyCheckError("Clear FBO 2");
    glEnable(GL_DEPTH_TEST);
    MyCheckError("Clear FBO 3");

    glEnable(GL_STENCIL_TEST);
    glClearStencil(0);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);

    switch (fbo_mode) {
      case 0: case 1:
        g_hellotriangle->Render(); break;
      case 2:
        glDisable(GL_DEPTH_TEST);
        const double sec = glfwGetTime();
        const int N = 1+(int(sec / 0.25)) % 255;
        for (int i=0; i<N; i++) {
          g_hellotriangle->Render();  // Stencil increments N times
        }
        break;
    }

    glDisable(GL_STENCIL_TEST);

    switch (fbo_mode) {
      case 0: depth_fbo->Unbind(); break;
      case 1:
        depth_fbo2->ReadDepthBufferIntoTexture();
        depth_fbo2->Unbind();
        break;
      case 2:
        depth_stencil_fbo->ReadBuffersIntoTexture();
        depth_stencil_fbo->Unbind();
        break;
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_tex);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float)*5, 0);
  glVertexAttribPointer(1, 2, GL_FLOAT, false, sizeof(float)*5, (void*)(3*sizeof(float)));
  
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);
  glUseProgram(shader_program);

  // To fix: segfault in /usr/lib/x86_64-linux-gnu/dri/zx_dri.so
  glViewport(0, 0, WIN_W/2, WIN_H/2);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glViewport(WIN_W/2, 0, WIN_W/2, WIN_H/2);
  glBindTexture(GL_TEXTURE_2D, basic_fbo->tex);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glViewport(0, WIN_H/2, WIN_W/2, WIN_H/2);
  glBindTexture(GL_TEXTURE_2D, depth_buffer_tex);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glViewport(WIN_W/2, WIN_H/2, WIN_W/2, WIN_H/2);
  
  switch (fbo_mode) {
    case 0: glBindTexture(GL_TEXTURE_2D, depth_fbo->tex); break;
    case 1: glBindTexture(GL_TEXTURE_2D, depth_fbo2->tex); break;
    case 2: {
      #ifndef USE_ALTERNATIVE_WAY_TO_DRAW_STENCIL
      glUseProgram(shader_program_stencil);
      glBindTexture(GL_TEXTURE_2D, depth_stencil_fbo->stencil_tex); break;  // D24S8 in this case
      #else
      glBindTexture(GL_TEXTURE_2D, depth_stencil_fbo->stencil_tex); break;  // RGBA8 in this case
      #endif
    }
  }
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glViewport(0, 0, WIN_W, WIN_H);
  
  MyCheckError("TextureScene Render");
}

void TextureScene::Update(float secs) {
  
}