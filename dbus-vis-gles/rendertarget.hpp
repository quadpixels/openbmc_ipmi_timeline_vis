#pragma once

//#define USE_ALTERNATIVE_WAY_TO_DRAW_STENCIL

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

  // To visualize the depth buffer, we create two FBOs and blit from #1 to #2 each time we render it
  // FBO #1: backed by a depth-only RenderBuffer
  // +-------------+   +--------------+
  // |     fbo     |---| depth_buffer |
  // +-------------+   +--------------+
  GLuint depth_buffer;
  // FBO #2: backed by a FBO texture, `tex`
  // +-------------+   +-----+
  // | fbo_texture |---| tex |
  // +-------------+   +-----+
  GLuint fbo_texture;
};

class DepthStencilFBO : public FBO {
public:
  DepthStencilFBO(const int _w, const int _h);
  void ReadBuffersIntoTexture();

  // FBO #1: backed by RenderBuffers
  // +-------------+   +----------------+
  // |    fbo      |---| d24s8 buffer   |
  // +-------------+   +----------------+
  //                   | color attachmt |
  //                   +----------------+

  // FBO #2: backed by a D24S8 texture viewed as stencil
  // +-------------+   +----------------+
  // | fbo_texture |---| d24s8 tex      |
  // +-------------+   +----------------+
  //                   | tex            |
  //                   +----------------+
  GLuint fbo_texture;
  GLuint stencil_tex;

  // Alternative way for showing the Stencil
  unsigned int vbo_quad, ebo_quad;
  unsigned int shader_program_quad;
};