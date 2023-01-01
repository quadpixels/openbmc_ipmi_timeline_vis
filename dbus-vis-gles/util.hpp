#pragma once
#include <string>
#include <glm/gtc/matrix_transform.hpp>

std::string ReadFileIntoString(const char* file_name);
unsigned int BuildShaderProgram(const char* vs_filename, const char* fs_filename);
void MyCheckShaderCompileStatus(unsigned int shader, const char* tag);
void MyCheckShaderProgramLinkStatus(unsigned int program, const char* tag);
void MyCheckError(const char* tag, bool ignore = false);
float Lerp(float a, float b, float t);
glm::mat3 RotateAroundLocalAxis(const glm::mat3& orientation, const glm::vec3& axis, const float deg);
glm::mat3 RotateAroundGlobalAxis(const glm::mat3& orientation, const glm::vec3& axis, const float deg);
void PrintMat4(const glm::mat4& m, const char* tag);
