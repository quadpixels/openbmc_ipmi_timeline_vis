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