#include "Shader.hpp"

#include <GL/glew.h>
#include <string>

#include <utility>

namespace gl {

Shader::Shader(GLenum type) : name_(glCreateShader(type))
{}

Shader::Shader(Shader&& other) : name_(other.name_) {
	other.name_ = 0;
}

Shader& Shader::operator=(Shader&& other) {
	std::swap(name_, other.name_);
	return *this;
}

Shader::~Shader() {
	glDeleteShader(name_);
}

void Shader::source(const std::string& src) {
	auto ptr = src.c_str();

	glShaderSource(name_, 1, &ptr, nullptr);
}

bool Shader::compile() {
	glCompileShader(name_);
	GLint status;
	glGetShaderiv(name_, GL_COMPILE_STATUS, &status);

	return status;
}

std::string Shader::infoLog() {
	GLint len;
	glGetShaderiv(name_, GL_INFO_LOG_LENGTH, &len);

	char* buf = new char[len];
	glGetShaderInfoLog(name_, len, nullptr, buf);

	std::string toRet = buf;
	delete[] buf;

	return toRet;
}

}
