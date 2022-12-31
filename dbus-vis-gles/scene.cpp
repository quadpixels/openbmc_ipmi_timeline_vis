#include <vector>

#include "util.hpp"
#include "scene.hpp"

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