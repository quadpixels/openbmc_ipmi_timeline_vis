#pragma once
#include <string>

std::string ReadFileIntoString(const char* file_name);
unsigned int BuildShaderProgram(const char* vs_filename, const char* fs_filename);
void MyCheckShaderCompileStatus(unsigned int shader, const char* tag);
void MyCheckShaderProgramLinkStatus(unsigned int program, const char* tag);
void MyCheckError(const char* tag, bool ignore = false);
float Lerp(float a, float b, float t);