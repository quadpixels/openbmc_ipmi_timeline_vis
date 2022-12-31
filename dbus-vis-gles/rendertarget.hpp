#pragma once

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

class FBO {
public:
  GLuint fbo, tex, width, height;
  void Bind();
  void Unbind();
};

class BasicFBO : public FBO {
public:
  BasicFBO(const int _w, const int _h);
  GLuint depth_tex;
};

class DepthOnlyFBO : public FBO {
public:
  DepthOnlyFBO(const int _w, const int _h);
};