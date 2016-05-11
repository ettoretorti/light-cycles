#pragma once

#include <GL/glew.h>
#include <string>

namespace gl {

class Shader {
friend class Program;

public:
	Shader(GLenum type);
	Shader(const Shader& other) = delete;
	Shader& operator=(const Shader& other) = delete;
	Shader(Shader&& other);
	Shader& operator=(Shader&& other);
	~Shader();

	void source(const std::string& src);
	bool compile();
	std::string infoLog();

private:
	GLuint name_;
};

}
