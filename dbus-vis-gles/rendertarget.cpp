#include "rendertarget.hpp"
#include "util.hpp"

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

  // 
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

  //
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

// Separate depth and stencil
DepthStencilFBO::DepthStencilFBO(const int _w, const int _h) {
  /*
  width = _w; height = _h;
  glGenTextures(1, &tex);  // ss
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
               width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenTextures(1, &stencil_tex);
  glBindTexture(GL_TEXTURE_2D, stencil_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8,
               width, height, 0, GL_STENCIL_INDEX8, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  MyCheckError("create depth+stencil fbo w/ separate D+S textures");
  */
}