#include <vector>

#include "rendertarget.hpp"
#include "scene.hpp"
#include "util.hpp"
#include <glm/gtc/matrix_transform.hpp>

extern int WIN_W, WIN_H;
extern HelloTriangleScene* g_hellotriangle;

HelloTriangleScene::HelloTriangleScene() {
  const float R0 = 1.0f, G0 = 0.4f, B0 = 0.4f;
  const float R1 = 0.4f, G1 = 1.0f, B1 = 0.4f;
  const float R2 = 0.4f, G2 = 0.4f, B2 = 1.0f;
  // 1. Buffer
  constexpr float vertices0[] = {
    -0.9f, -0.5f, 0.0f, R0, G0, B0,
    -0.1f, -0.5f, 0.0f, R1, G1, B1,
    -0.5f,  0.5f, 0.0f, R2, G2, B2,
  };

  glGenBuffers(1, &vbo_solid);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_solid);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices0), vertices0, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  constexpr float vertices1[] = {
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
  glLineWidth(2);
  glDrawArrays(GL_LINES, 0, 6);
  
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glUseProgram(0);
  MyCheckError("Render");
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
  glDrawElements(GL_LINES, positions.size()*2, GL_UNSIGNED_INT, 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisableVertexAttribArray(0);
  glDisableVertexAttribArray(1);
  glUseProgram(0);
  MyCheckError("RotatingCubeScene Render");
}

OneChunkScene::OneChunkScene() {
  // Backdrop
  const float L = 32;
  const float cidx = 120;  // color idx for backdrop
  const float verts[] = {
    -L, -20, -L, 4, cidx, 0,
    -L, -20,  L, 4, cidx, 0,
     L, -20,  L, 4, cidx, 0,
     L, -20, -L, 4, cidx, 0,
  };
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  const int idxes[] = {
    3,0,1,  1,2,3
  };
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxes), idxes, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

  // Chunk
  chunk.LoadDefault();
  Chunk* null_neighs[26] = {};
  chunk.BuildBuffers(null_neighs);
  MyCheckError("Chunk Build Default");

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
  chunk.Render();
  depth_fbo->Unbind();
  MyCheckError("Render to Depth FBO");

  glUseProgram(Chunk::shader_program);
  glm::mat4 lightPV = directional_light->P * directional_light->V;
  glUniformMatrix4fv(lightpv_loc, 1, false, &lightPV[0][0]);
  V = camera.GetViewMatrix();
  P = projection_matrix;
  glUniformMatrix4fv(v_loc, 1, false, &V[0][0]);
  glUniformMatrix4fv(p_loc, 1, false, &P[0][0]);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, depth_fbo->tex);
  // Drawing the backdrop
  {
    unsigned m_loc = glGetUniformLocation(Chunk::shader_program, "M");
    glm::mat4 M(1);
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
  }
  MyCheckError("Drawing the backdrop");

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, depth_fbo->tex);
  chunk.Render();
  MyCheckError("Chunk render");
}

void OneChunkScene::Update(float secs) {

}

TextureScene::TextureScene() {
  // 1. Shader, VBO, EBO
  shader_program = BuildShaderProgram("shaders/simple_texture.vs", "shaders/simple_texture.fs");
  MyCheckError("build TextureScene shaders");

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
  basic_fbo = new BasicFBO(WIN_W, WIN_H);
  depth_fbo = new DepthOnlyFBO(WIN_W, WIN_H);
}

void TextureScene::Render() {

  {
    basic_fbo->Bind();
    g_hellotriangle->Render();
    basic_fbo->Unbind();
  }

  {
    depth_fbo->Bind();
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    g_hellotriangle->Render();
    depth_fbo->Unbind();
  }

  glBindBuffer(GL_ARRAY_BUFFER, vbo_tex);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_tex);
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, false, sizeof(float)*5, 0);
  glVertexAttribPointer(1, 3, GL_FLOAT, false, sizeof(float)*5, (void*)(3*sizeof(float)));
  
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
  glBindTexture(GL_TEXTURE_2D, depth_fbo->tex);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glViewport(0, 0, WIN_W, WIN_H);
  MyCheckError("TextureScene Render");
}

void TextureScene::Update(float secs) {

}
