
#include "Buffer.hpp"
#include <GL/glew.h>

#include <utility>

namespace gl {

//state shadowing to avoid needless binding
static thread_local GLuint curBufs[15] = { 0 };
static GLuint& curBuf(GLenum target) {
	GLuint idx = 0;

	switch(target) {
		case GL_ARRAY_BUFFER:              idx =  1; break;
		case GL_ATOMIC_COUNTER_BUFFER:     idx =  2; break;
		case GL_COPY_READ_BUFFER:          idx =  3; break;
		case GL_COPY_WRITE_BUFFER:         idx =  4; break;
		case GL_DISPATCH_INDIRECT_BUFFER:  idx =  5; break;
		case GL_DRAW_INDIRECT_BUFFER:      idx =  6; break;
		case GL_ELEMENT_ARRAY_BUFFER:      idx =  7; break;
		case GL_PIXEL_PACK_BUFFER:         idx =  8; break;
		case GL_PIXEL_UNPACK_BUFFER:       idx =  9; break;
		case GL_QUERY_BUFFER:              idx = 10; break;
		case GL_SHADER_STORAGE_BUFFER:     idx = 11; break;
		case GL_TEXTURE_BUFFER:            idx = 12; break;
		case GL_TRANSFORM_FEEDBACK_BUFFER: idx = 13; break;
		case GL_UNIFORM_BUFFER:            idx = 14; break;
		default:                           idx =  0; break;
	}

	return curBufs[idx];
}
static void bufBind(GLenum target, GLuint name) {
	auto& cBuf = curBuf(target);
	if(cBuf != name) {
	  glBindBuffer(target, name);
	  cBuf = name;
	}
}

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
	bufBind(target_, name_);
}

void Buffer::data(GLsizeiptr size, const GLvoid* data, GLenum usage) {
	bufBind(GL_COPY_WRITE_BUFFER, name_);
	glBufferData(GL_COPY_WRITE_BUFFER, size, data, usage);
}

void Buffer::subData(GLintptr offset, GLsizeiptr size, const GLvoid* data) {
	bufBind(GL_COPY_WRITE_BUFFER, name_);
	glBufferSubData(GL_COPY_WRITE_BUFFER, offset, size, data);
}

}
