#include "rendertarget.hpp"
#include "util.hpp"

// 对于 ES 3.0，没有 GL_DEPTH_STENCIL_ATTACHMENT 可用，另寻他法
#ifndef USE_ALTERNATIVE_WAY_TO_DRAW_STENCIL
#include <GLES3/gl31.h>
#endif

extern int WIN_W, WIN_H;

void FBO::Bind() {
  glViewport(0, 0, width, height);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void FBO::Unbind() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

BasicFBO::BasicFBO(const int _w, const int _h) {
  width = _w; height = _h;

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
      width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glGenTextures(1, &depth_tex);
  glBindTexture(GL_TEXTURE_2D, depth_tex);

  // https://kripken.github.io/emscripten-site/docs/optimizing/Optimizing-WebGL.html
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
      width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);

  // The following 2 lines are needed
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,  GL_TEXTURE_2D, depth_tex, 0);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

  //GLenum rts[1] = { GL_COLOR_ATTACHMENT0 };
  //glDrawBuffers(1, rts);

  MyCheckError("create singlesample fbo");

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    printf("Single-sampled FBO not complete.\n");
    assert(0);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

DepthOnlyFBO::DepthOnlyFBO(const int _w, const int _h) {
  width = _w; height = _h;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
               width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL); // changed for js

  // filtering method must match texture's filter-ability
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  MyCheckError("create depth fbo (using texture)");
}

DepthOnlyFBO2::DepthOnlyFBO2(const int _w, const int _h) {
  width = _w; height = _h;

  // Texture-backed FBO for visualization
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
               width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
  
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &fbo_texture);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_texture);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  // RenderBuffer-backed FBO for rendering
  glGenRenderbuffers(1, &depth_buffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);

  glGenFramebuffers(1, &fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buffer);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  MyCheckError("create depth fbo v2 (using frame buffer)");
}

void DepthOnlyFBO2::ReadDepthBufferIntoTexture() {
  // 以下这三个操作需要ES3，ES2不行
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_texture);
  glBlitFramebuffer(0, 0, width, height,
    0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

// Separate depth and stencil FBOs backed by RenderBuffer
DepthStencilFBO::DepthStencilFBO(const int _w, const int _h) {
  width = _w; height = _h;
  GLenum fbo_status;
  // Texture-backed FBO for visualization
  #ifndef USE_ALTERNATIVE_WAY_TO_DRAW_STENCIL
  {
    glGenTextures(1, &stencil_tex);
    glBindTexture(GL_TEXTURE_2D, stencil_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
                width, height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);  // View the stencil part of this tex
    MyCheckError("create depth stencil fbo depth tex");
  }

  glGenFramebuffers(1, &fbo_texture);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_texture);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencil_tex, 0);
  fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbo_status != GL_FRAMEBUFFER_COMPLETE) {
    printf("Depth-Stencil tex FBO status: %X\n", fbo_status);
    exit(1);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  MyCheckError("create depth stencil fbo texs");
  #else
  glGenTextures(1, &stencil_tex);
  glBindTexture(GL_TEXTURE_2D, stencil_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
      width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

  glGenFramebuffers(1, &fbo_texture);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_texture);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, stencil_tex, 0);
  fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbo_status != GL_FRAMEBUFFER_COMPLETE) {
    printf("Depth-Stencil tex FBO for alternative Stencil visualization method status: %X\n", fbo_status);
    exit(1);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  MyCheckError("create depth stencil fbo texs");
  #endif

  // RenderBufer-backed FBO for rendering
  {
    GLuint ds_buffer;
    glGenRenderbuffers(1, &ds_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, ds_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    GLuint color_buffer;
    glGenRenderbuffers(1, &color_buffer);
    glBindRenderbuffer(GL_RENDERBUFFER, color_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ds_buffer);
  }
  fbo_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (fbo_status != GL_FRAMEBUFFER_COMPLETE) {
    printf("Depth-Stencil FBO status: %X\n", fbo_status);
    exit(1);
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  MyCheckError("create depth stencil fbo (using frame buffer)");

  shader_program_quad = BuildShaderProgram("shaders/quad.vs", "shaders/hellotriangle.fs");
  float verts1[] = {
    -1,  1,
    -1, -1,
     1, -1,
     1,  1,
  };
  glGenBuffers(1, &vbo_quad);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts1), verts1, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  MyCheckError("Create VBO for quad");

  unsigned int idxes[] = { 0,1,3,  3,1,2 };
  glGenBuffers(1, &ebo_quad);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_quad);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idxes), idxes, GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  MyCheckError("create quad resources");
}

void DepthStencilFBO::ReadBuffersIntoTexture() {
  #ifdef USE_ALTERNATIVE_WAY_TO_DRAW_STENCIL
  glm::vec3 palette[] = {
    glm::vec3(0.0,0.0,0.0),glm::vec3(1.0,1.0,1.0),glm::vec3(0.8,1.0,1.0),glm::vec3(0.6,1.0,1.0),glm::vec3(0.4,1.0,1.0),glm::vec3(0.2,1.0,1.0),glm::vec3(0.0,1.0,1.0),glm::vec3(1.0,0.8,1.0),
    glm::vec3(0.8,0.8,1.0),glm::vec3(0.6,0.8,1.0),glm::vec3(0.4,0.8,1.0),glm::vec3(0.2,0.8,1.0),glm::vec3(0.0,0.8,1.0),glm::vec3(1.0,0.6,1.0),glm::vec3(0.8,0.6,1.0),glm::vec3(0.6,0.6,1.0),
    glm::vec3(0.4,0.6,1.0),glm::vec3(0.2,0.6,1.0),glm::vec3(0.0,0.6,1.0),glm::vec3(1.0,0.4,1.0),glm::vec3(0.8,0.4,1.0),glm::vec3(0.6,0.4,1.0),glm::vec3(0.4,0.4,1.0),glm::vec3(0.2,0.4,1.0),
    glm::vec3(0.0,0.4,1.0),glm::vec3(1.0,0.2,1.0),glm::vec3(0.8,0.2,1.0),glm::vec3(0.6,0.2,1.0),glm::vec3(0.4,0.2,1.0),glm::vec3(0.2,0.2,1.0),glm::vec3(0.0,0.2,1.0),glm::vec3(1.0,0.0,1.0),
    glm::vec3(0.8,0.0,1.0),glm::vec3(0.6,0.0,1.0),glm::vec3(0.4,0.0,1.0),glm::vec3(0.2,0.0,1.0),glm::vec3(0.0,0.0,1.0),glm::vec3(1.0,1.0,0.8),glm::vec3(0.8,1.0,0.8),glm::vec3(0.6,1.0,0.8),
    glm::vec3(0.4,1.0,0.8),glm::vec3(0.2,1.0,0.8),glm::vec3(0.0,1.0,0.8),glm::vec3(1.0,0.8,0.8),glm::vec3(0.8,0.8,0.8),glm::vec3(0.6,0.8,0.8),glm::vec3(0.4,0.8,0.8),glm::vec3(0.2,0.8,0.8),
    glm::vec3(0.0,0.8,0.8),glm::vec3(1.0,0.6,0.8),glm::vec3(0.8,0.6,0.8),glm::vec3(0.6,0.6,0.8),glm::vec3(0.4,0.6,0.8),glm::vec3(0.2,0.6,0.8),glm::vec3(0.0,0.6,0.8),glm::vec3(1.0,0.4,0.8),
    glm::vec3(0.8,0.4,0.8),glm::vec3(0.6,0.4,0.8),glm::vec3(0.4,0.4,0.8),glm::vec3(0.2,0.4,0.8),glm::vec3(0.0,0.4,0.8),glm::vec3(1.0,0.2,0.8),glm::vec3(0.8,0.2,0.8),glm::vec3(0.6,0.2,0.8),
    glm::vec3(0.4,0.2,0.8),glm::vec3(0.2,0.2,0.8),glm::vec3(0.0,0.2,0.8),glm::vec3(1.0,0.0,0.8),glm::vec3(0.8,0.0,0.8),glm::vec3(0.6,0.0,0.8),glm::vec3(0.4,0.0,0.8),glm::vec3(0.2,0.0,0.8),
    glm::vec3(0.0,0.0,0.8),glm::vec3(1.0,1.0,0.6),glm::vec3(0.8,1.0,0.6),glm::vec3(0.6,1.0,0.6),glm::vec3(0.4,1.0,0.6),glm::vec3(0.2,1.0,0.6),glm::vec3(0.0,1.0,0.6),glm::vec3(1.0,0.8,0.6),
    glm::vec3(0.8,0.8,0.6),glm::vec3(0.6,0.8,0.6),glm::vec3(0.4,0.8,0.6),glm::vec3(0.2,0.8,0.6),glm::vec3(0.0,0.8,0.6),glm::vec3(1.0,0.6,0.6),glm::vec3(0.8,0.6,0.6),glm::vec3(0.6,0.6,0.6),
    glm::vec3(0.4,0.6,0.6),glm::vec3(0.2,0.6,0.6),glm::vec3(0.0,0.6,0.6),glm::vec3(1.0,0.4,0.6),glm::vec3(0.8,0.4,0.6),glm::vec3(0.6,0.4,0.6),glm::vec3(0.4,0.4,0.6),glm::vec3(0.2,0.4,0.6),
    glm::vec3(0.0,0.4,0.6),glm::vec3(1.0,0.2,0.6),glm::vec3(0.8,0.2,0.6),glm::vec3(0.6,0.2,0.6),glm::vec3(0.4,0.2,0.6),glm::vec3(0.2,0.2,0.6),glm::vec3(0.0,0.2,0.6),glm::vec3(1.0,0.0,0.6),
    glm::vec3(0.8,0.0,0.6),glm::vec3(0.6,0.0,0.6),glm::vec3(0.4,0.0,0.6),glm::vec3(0.2,0.0,0.6),glm::vec3(0.0,0.0,0.6),glm::vec3(1.0,1.0,0.4),glm::vec3(0.8,1.0,0.4),glm::vec3(0.6,1.0,0.4),
    glm::vec3(0.4,1.0,0.4),glm::vec3(0.2,1.0,0.4),glm::vec3(0.0,1.0,0.4),glm::vec3(1.0,0.8,0.4),glm::vec3(0.8,0.8,0.4),glm::vec3(0.6,0.8,0.4),glm::vec3(0.4,0.8,0.4),glm::vec3(0.2,0.8,0.4),
    glm::vec3(0.0,0.8,0.4),glm::vec3(1.0,0.6,0.4),glm::vec3(0.8,0.6,0.4),glm::vec3(0.6,0.6,0.4),glm::vec3(0.4,0.6,0.4),glm::vec3(0.2,0.6,0.4),glm::vec3(0.0,0.6,0.4),glm::vec3(1.0,0.4,0.4),
    glm::vec3(0.8,0.4,0.4),glm::vec3(0.6,0.4,0.4),glm::vec3(0.4,0.4,0.4),glm::vec3(0.2,0.4,0.4),glm::vec3(0.0,0.4,0.4),glm::vec3(1.0,0.2,0.4),glm::vec3(0.8,0.2,0.4),glm::vec3(0.6,0.2,0.4),
    glm::vec3(0.4,0.2,0.4),glm::vec3(0.2,0.2,0.4),glm::vec3(0.0,0.2,0.4),glm::vec3(1.0,0.0,0.4),glm::vec3(0.8,0.0,0.4),glm::vec3(0.6,0.0,0.4),glm::vec3(0.4,0.0,0.4),glm::vec3(0.2,0.0,0.4),
    glm::vec3(0.0,0.0,0.4),glm::vec3(1.0,1.0,0.2),glm::vec3(0.8,1.0,0.2),glm::vec3(0.6,1.0,0.2),glm::vec3(0.4,1.0,0.2),glm::vec3(0.2,1.0,0.2),glm::vec3(0.0,1.0,0.2),glm::vec3(1.0,0.8,0.2),
    glm::vec3(0.8,0.8,0.2),glm::vec3(0.6,0.8,0.2),glm::vec3(0.4,0.8,0.2),glm::vec3(0.2,0.8,0.2),glm::vec3(0.0,0.8,0.2),glm::vec3(1.0,0.6,0.2),glm::vec3(0.8,0.6,0.2),glm::vec3(0.6,0.6,0.2),
    glm::vec3(0.4,0.6,0.2),glm::vec3(0.2,0.6,0.2),glm::vec3(0.0,0.6,0.2),glm::vec3(1.0,0.4,0.2),glm::vec3(0.8,0.4,0.2),glm::vec3(0.6,0.4,0.2),glm::vec3(0.4,0.4,0.2),glm::vec3(0.2,0.4,0.2),
    glm::vec3(0.0,0.4,0.2),glm::vec3(1.0,0.2,0.2),glm::vec3(0.8,0.2,0.2),glm::vec3(0.6,0.2,0.2),glm::vec3(0.4,0.2,0.2),glm::vec3(0.2,0.2,0.2),glm::vec3(0.0,0.2,0.2),glm::vec3(1.0,0.0,0.2),
    glm::vec3(0.8,0.0,0.2),glm::vec3(0.6,0.0,0.2),glm::vec3(0.4,0.0,0.2),glm::vec3(0.2,0.0,0.2),glm::vec3(0.0,0.0,0.2),glm::vec3(1.0,1.0,0.0),glm::vec3(0.8,1.0,0.0),glm::vec3(0.6,1.0,0.0),
    glm::vec3(0.4,1.0,0.0),glm::vec3(0.2,1.0,0.0),glm::vec3(0.0,1.0,0.0),glm::vec3(1.0,0.8,0.0),glm::vec3(0.8,0.8,0.0),glm::vec3(0.6,0.8,0.0),glm::vec3(0.4,0.8,0.0),glm::vec3(0.2,0.8,0.0),
    glm::vec3(0.0,0.8,0.0),glm::vec3(1.0,0.6,0.0),glm::vec3(0.8,0.6,0.0),glm::vec3(0.6,0.6,0.0),glm::vec3(0.4,0.6,0.0),glm::vec3(0.2,0.6,0.0),glm::vec3(0.0,0.6,0.0),glm::vec3(1.0,0.4,0.0),
    glm::vec3(0.8,0.4,0.0),glm::vec3(0.6,0.4,0.0),glm::vec3(0.4,0.4,0.0),glm::vec3(0.2,0.4,0.0),glm::vec3(0.0,0.4,0.0),glm::vec3(1.0,0.2,0.0),glm::vec3(0.8,0.2,0.0),glm::vec3(0.6,0.2,0.0),
    glm::vec3(0.4,0.2,0.0),glm::vec3(0.2,0.2,0.0),glm::vec3(0.0,0.2,0.0),glm::vec3(1.0,0.0,0.0),glm::vec3(0.8,0.0,0.0),glm::vec3(0.6,0.0,0.0),glm::vec3(0.4,0.0,0.0),glm::vec3(0.2,0.0,0.0),
    glm::vec3(0.0,0.0,0.9),glm::vec3(0.0,0.0,0.9),glm::vec3(0.0,0.0,0.7),glm::vec3(0.0,0.0,0.7),glm::vec3(0.0,0.0,0.5),glm::vec3(0.0,0.0,0.5),glm::vec3(0.0,0.0,0.3),glm::vec3(0.0,0.0,0.3),
    glm::vec3(0.0,0.0,0.1),glm::vec3(0.0,0.0,0.1),glm::vec3(0.0,0.9,0.0),glm::vec3(0.0,0.9,0.0),glm::vec3(0.0,0.7,0.0),glm::vec3(0.0,0.7,0.0),glm::vec3(0.0,0.5,0.0),glm::vec3(0.0,0.5,0.0),
    glm::vec3(0.0,0.3,0.0),glm::vec3(0.0,0.3,0.0),glm::vec3(0.0,0.1,0.0),glm::vec3(0.0,0.1,0.0),glm::vec3(0.9,0.0,0.0),glm::vec3(0.9,0.0,0.0),glm::vec3(0.7,0.0,0.0),glm::vec3(0.7,0.0,0.0),
    glm::vec3(0.5,0.0,0.0),glm::vec3(0.5,0.0,0.0),glm::vec3(0.3,0.0,0.0),glm::vec3(0.3,0.0,0.0),glm::vec3(0.1,0.0,0.0),glm::vec3(0.1,0.0,0.0),glm::vec3(0.9,0.9,0.9),glm::vec3(0.9,0.9,0.9),
    glm::vec3(0.7,0.7,0.7),glm::vec3(0.7,0.7,0.7),glm::vec3(0.5,0.5,0.5),glm::vec3(0.5,0.5,0.5),glm::vec3(0.3,0.3,0.3),glm::vec3(0.3,0.3,0.3),glm::vec3(0.1,0.1,0.1),glm::vec3(0.1,0.1,0.1)
  };
  Bind();
  glViewport(0, 0, WIN_W, WIN_H);
  glEnable(GL_STENCIL_TEST);
  glDisable(GL_DEPTH_TEST);
  glDisableVertexAttribArray(1);
  glUseProgram(shader_program_quad);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_quad);
  glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float)*2, 0); // 这个要在bind buffer之后
  glEnableVertexAttribArray(0);
  GLuint c = glGetUniformLocation(shader_program_quad, "color");
  MyCheckError("set up quad draw");
  for (int i=0; i<256; i++) {
    glStencilFunc(GL_EQUAL, i, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glm::vec3 p = palette[i];
    glUniform3f(c, p.x, p.y, p.z);
    MyCheckError("glUniform3f");
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  }
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glDisable(GL_STENCIL_TEST);
  Unbind();
  #endif
  // 以下这三个操作需要ES3，ES2不行
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo_texture);
  #ifndef USE_ALTERNATIVE_WAY_TO_DRAW_STENCIL
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);  // 此时这个FBO只有Depth和Stencil，所以不能设置COLOR_BUFFER_BIT
  #else
  glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT | GL_COLOR_BUFFER_BIT, GL_NEAREST); // 此时这个FBO，Depth、Stencil、Color都具备
  #endif
  MyCheckError("blit depth+stencil buf");
}