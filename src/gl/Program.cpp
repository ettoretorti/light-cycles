#include "Program.hpp"

#include <GL/glew.h>
#include <string>

#include <utility>

namespace gl {

Program::Program() : name_(glCreateProgram())
{}

Program::Program(Program&& other) : name_(other.name_)
{
	other.name_ = 0;
}

Program& Program::operator=(Program&& other) {
	std::swap(name_, other.name_);
	return *this;
}

Program::~Program() {
	glDeleteProgram(name_);
}

bool Program::setVertexShader(const Shader& s) {
	GLint type;
	glGetShaderiv(s.name_, GL_SHADER_TYPE, &type);

	if(type != GL_VERTEX_SHADER) return false;

	glAttachShader(name_, s.name_);
	return true;
}

bool Program::setFragmentShader(const Shader& s) {
	GLint type;
	glGetShaderiv(s.name_, GL_SHADER_TYPE, &type);

	if(type != GL_FRAGMENT_SHADER) return false;

	glAttachShader(name_, s.name_);
	return true;
}

bool Program::link() {
	glLinkProgram(name_);

	GLint status;
	glGetProgramiv(name_, GL_LINK_STATUS, &status);

	return status;
}

std::string Program::infoLog() {
	GLint len;
	glGetProgramiv(name_, GL_INFO_LOG_LENGTH, &len);

	char* buf = new char[len];
	glGetProgramInfoLog(name_, len, nullptr, buf);

	std::string toRet = buf;
	delete[] buf;

	return toRet;
}

void Program::bindAttrib(GLuint idx, const std::string& name) {
	glBindAttribLocation(name_, idx, name.c_str());
}

GLint Program::getAttrib(const std::string& name) {
	return glGetAttribLocation(name_, name.c_str());
}

GLint Program::getUniform(const std::string& name) {
	return glGetUniformLocation(name_, name.c_str());
}

void Program::use() {
	glUseProgram(name_);
}

}
