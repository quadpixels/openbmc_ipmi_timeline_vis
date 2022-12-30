#include <string>

std::string ReadFileIntoString(const char* file_name);
void MyCheckShaderCompileStatus(unsigned int shader, const char* tag);
void MyCheckShaderProgramLinkStatus(unsigned int program, const char* tag);