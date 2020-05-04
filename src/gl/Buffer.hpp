#pragma once

#include <glad/glad.h>

namespace gl {

class Buffer {
public:
	Buffer(GLenum target);
	Buffer(const Buffer& other) = delete;
	Buffer& operator=(const Buffer& other) = delete;
	Buffer(Buffer&& other);
	Buffer& operator=(Buffer&& other);
	~Buffer();

	GLenum& target();
	const GLenum& target() const;
	void bind() const;

	void data(GLsizeiptr size, const GLvoid* data, GLenum usage);
	void subData(GLintptr offset, GLsizeiptr size, const GLvoid* data);

private:
	GLuint name_;
	GLenum target_;
};

}
