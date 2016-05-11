#pragma once

#include <GL/glew.h>
#include <string>

#include "Shader.hpp"

namespace gl {

class Program {
public:
	Program();
	Program(const Program& other) = delete;
	Program& operator=(const Program& other) = delete;
	Program(Program&& other);
	Program& operator=(Program&& other);
	~Program();

	bool setVertexShader(const Shader& s);
	bool setFragmentShader(const Shader& s);
	bool link();
	std::string infoLog();

	void bindAttrib(GLuint idx, const std::string& name);
	GLint getAttrib(const std::string& name);

	GLint getUniform(const std::string& name);

	void use();

private:
	GLuint name_;
};

}
