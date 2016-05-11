
#include "Buffer.hpp"
#include <GL/glew.h>

#include <utility>

namespace gl {

Buffer::Buffer(GLenum target) : name_(0), target_(target)
{
	glGenBuffers(1, &name_);
}

Buffer::Buffer(Buffer&& other) : name_(other.name_), target_(other.target_)
{
	other.name_ = 0;
}

Buffer& Buffer::operator=(Buffer&& other) {
	std::swap(name_, other.name_);
	return *this;
}

const GLenum& Buffer::target() const {
	return target_;
}

GLenum& Buffer::target() {
	return target_;
}

Buffer::~Buffer() {
	glDeleteBuffers(1, &name_);
}

void Buffer::bind() const {
	glBindBuffer(target_, name_);
}

void Buffer::data(GLsizeiptr size, const GLvoid* data, GLenum usage) {
	glBindBuffer(GL_COPY_WRITE_BUFFER, name_);
	glBufferData(GL_COPY_WRITE_BUFFER, size, data, usage);
}

void Buffer::subData(GLintptr offset, GLsizeiptr size, const GLvoid* data) {
	glBindBuffer(GL_COPY_WRITE_BUFFER, name_);
	glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
}

}
