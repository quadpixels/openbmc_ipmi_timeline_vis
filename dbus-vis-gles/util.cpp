#include <fstream>
#include <string>

#define GLFW_INCLUDE_ES2
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

std::string ReadFileIntoString(const char* file_name) {
  std::string ret;
  std::ifstream ifs(file_name);
  if (ifs.is_open()) {
    std::string line;
    while (getline(ifs, line)) {
      ret += line + "\n";
    }
    ifs.close();
  } else {
    printf("Error: cannot open %s\n", file_name);
    exit(1);
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
  if (!ignore) {
    exit(1);
    glfwTerminate();  // To make sure program really terminates in Emscripten
  }
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

  std::string fs_src = ReadFileIntoString(fs_filename);
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

glm::mat3 RotateAroundLocalAxis(const glm::mat3& orientation, const glm::vec3& axis, const float deg) {
  glm::mat4 o(orientation);
  o = glm::rotate(o, deg*3.14159f/180.0f, glm::inverse(orientation)*axis);
  return glm::mat3(o);
}

glm::mat3 RotateAroundGlobalAxis(const glm::mat3& orientation, const glm::vec3& axis, const float deg) {
  glm::mat4 o(orientation);
  o = glm::rotate(o, deg*3.14159f/180.0f, axis);
  return glm::mat3(o);
}

void PrintMat4(const glm::mat4& m, const char* tag) {
  printf("%s =\n", tag);
  for (int i=0; i<4; i++)
    printf("%5g, %5g, %5g, %5g\n", m[i][0], m[i][1], m[i][2], m[i][3]);
}

float RandRange(float lb, float ub) {
  return float(lb + (ub-lb) * rand() / (double)RAND_MAX);
}