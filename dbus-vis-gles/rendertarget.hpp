#pragma once

#define GLFW_INCLUDE_ES3
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

// 用Texture
class DepthOnlyFBO : public FBO {
public:
  DepthOnlyFBO(const int _w, const int _h);
};

// 用RenderBuffer
class DepthOnlyFBO2 : public FBO {
public:
  DepthOnlyFBO2(const int _w, const int _h);
  void ReadDepthBufferIntoTexture();

  GLuint depth_buffer;
  GLuint fbo_texture;
};

class DepthStencilFBO : public FBO {
public:
  DepthStencilFBO(const int _w, const int _h);
  GLuint stencil_tex;
};