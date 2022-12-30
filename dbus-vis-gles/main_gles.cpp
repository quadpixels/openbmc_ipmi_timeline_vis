#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

#include "util.hpp"

GLFWwindow* g_window;
double g_last_secs = 0;

const int WIN_W = 1024, WIN_H = 640;

void MyCheckError(const char* tag, bool ignore = false) {
  GLenum err = glGetError();
  if (err == GL_NO_ERROR) return;
  switch (err) {
    case GL_INVALID_ENUM: printf("GL_INVALID_ENUM, %s\n", tag); break;
    case GL_INVALID_VALUE: printf("GL_INVALID_VALUE, %s\n", tag); break;
    case GL_INVALID_OPERATION: printf("GL_INVALID_OPERATION, %s\n", tag); break;
    case GL_INVALID_FRAMEBUFFER_OPERATION: printf("GL_INVALID_FRAMEBUFFER_OPERATION, %s\n", tag); break;
    case GL_OUT_OF_MEMORY: printf("GL_OUT_OF_MEMORY, %s\n", tag); break;
    default: printf("Error %d: %s\n", err, tag); break;
  }
  if (!ignore) exit(1);
}

class HelloTriangleScene {
public:
  unsigned int vbo_solid, vbo_line; // Buffer

  // Shader Program = VS + FS.
  unsigned int shader_program;
  HelloTriangleScene() {
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
    int vertex_shader, fragment_shader;

    std::string vs_src = ReadFileIntoString("shaders/hellotriangle.vs");
    const char* vs_src_data = vs_src.data();
    printf("VS source: %zu bytes\n", vs_src.size());
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vs_src_data, NULL);
    glCompileShader(vertex_shader);
    MyCheckError("Compile VS");
    MyCheckShaderCompileStatus(vertex_shader, "Vertex shader");

    std::string fs_src = ReadFileIntoString("shaders/hellotriangle.fs");
    const char* fs_src_data = fs_src.data();
    printf("FS source: %zu bytes\n", fs_src.size());
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fs_src_data, NULL);
    glCompileShader(fragment_shader);
    MyCheckError("Compile FS");
    MyCheckShaderCompileStatus(fragment_shader, "Fragment shader");

    shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    MyCheckShaderProgramLinkStatus(shader_program, "Shader program");

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
  }

  void Render() {
    glUseProgram(shader_program);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_solid);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), NULL);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
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
    glUseProgram(0);
    MyCheckError("Render");
  }
};

HelloTriangleScene* g_hellotriangle;

void Render() {
  glViewport(0, 0, WIN_W, WIN_H);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glClearColor(1.0f, 1.0f, 0.9f, 1.0f);
  glDisable(GL_DEPTH_TEST);

  g_hellotriangle->Render();
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE) {
    printf("Done.\n");
    exit(0);
  }
}

void MyInit() {
  g_hellotriangle = new HelloTriangleScene();
}

void emscriptenLoop() {
  double secs = glfwGetTime();
  glfwPollEvents();
  Render();
  glfwSwapBuffers(g_window);
  g_last_secs = secs;
}

int main(int argc, char** argv) {
  if (glfwInit() == GLFW_FALSE) {
    printf("glfwInit returned false\n");
    exit(1);
  }

  MyCheckError("glfwInit");

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  MyCheckError("set window hint");

  g_window = glfwCreateWindow(WIN_W, WIN_H, __FILE__, NULL, NULL);
  MyCheckError("created window");

  // https://discourse.glfw.org/t/invalid-enum-after-glfwmakecontextcurrent/170
  // Happens on ZX C960, ignoring
  glfwMakeContextCurrent(g_window);
  MyCheckError("make context current", true);

  glfwSetKeyCallback(g_window, KeyCallback);

  int major, minor, revision;
  glfwGetVersion(&major, &minor, &revision);
  printf("GLFW version: %d.%d.%d\n", major, minor, revision);
  const char* sz = (const char*)glGetString(GL_VERSION);
  printf("OpenGL Version String: %s\n", sz);
  const char* sz1 = (const char*)glGetString(GL_VENDOR);
  printf("OpenGL Vendor String: %s\n", sz1);
  const char* sz2 = (const char*)glGetString(GL_RENDERER);
  printf("OpenGL Renderer String: %s\n", sz2);
  const char* sz3 = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
  printf("OpenGL Shading Language Version: %s\n", sz3);
  
  MyCheckError("print a few strings");

  MyInit();

  #ifndef __EMSCRIPTEN__
  while (!glfwWindowShouldClose(g_window)) {
    emscriptenLoop();
  }
  #else
  emscripten_set_main_loop(emscriptenLoop, 0, 1);
  #endif

  printf("Done!\n");
  exit(0);
}