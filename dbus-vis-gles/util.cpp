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