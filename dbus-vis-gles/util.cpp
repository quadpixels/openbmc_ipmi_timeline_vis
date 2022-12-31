#include <fstream>
#include <string>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>

std::string ReadFileIntoString(const char* file_name) {
  std::string ret;
  std::ifstream ifs(file_name);
  while (ifs.good()) {
    ret.push_back(ifs.get());
  }
  return ret;
}

void MyCheckShaderCompileStatus(unsigned int shader, const char* tag) {
  int ret;
  char info[512];
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ret);
  if (!ret) {
    glGetShaderInfoLog(shader, sizeof(info), NULL, info);
    printf("Shader compile failed: %s\n%s\n", tag, info);
    exit(1);
  }
}

void MyCheckShaderProgramLinkStatus(unsigned int program, const char* tag) {
  int ret;
  char info[512];
  glGetProgramiv(program, GL_LINK_STATUS, &ret);
  if (!ret) {
    glGetProgramInfoLog(program, sizeof(info), NULL, info);
    printf("Program link failed: %s\n%s\n", tag, info);
    exit(1);
  }
}

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

float Lerp(float a, float b, float t) {
  return b*t + a*(1.0f-t);
}

unsigned int BuildShaderProgram(const char* vs_filename, const char* fs_filename) {
    int vertex_shader, fragment_shader;
  unsigned int shader_program;
  std::string vs_src = ReadFileIntoString(vs_filename);
  const char* vs_src_data = vs_src.data();
  printf("VS source %s: %zu bytes\n", vs_filename, vs_src.size());
  vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vs_src_data, NULL);
  glCompileShader(vertex_shader);
  MyCheckError("Compile VS");
  MyCheckShaderCompileStatus(vertex_shader, "Vertex shader");

  std::string fs_src = ReadFileIntoString("shaders/hellotriangle.fs");
  const char* fs_src_data = fs_src.data();
  printf("FS source %s: %zu bytes\n", fs_filename, fs_src.size());
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
  return shader_program;
}