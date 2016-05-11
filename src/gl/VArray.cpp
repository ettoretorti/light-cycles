
#include "VArray.hpp"
#include <GL/glew.h>
#include <utility>

namespace gl {

VArray::VArray() : name_(0)
{
	glGenVertexArrays(1, &name_);
}

VArray::VArray(VArray&& other) : name_(other.name_)
{
	other.name_ = 0;
}

VArray& VArray::operator=(VArray&& other) {
	std::swap(name_, other.name_);
	return *this;
}

VArray::~VArray() {
	glDeleteVertexArrays(1, &name_);
}

void VArray::bind() const {
	glBindVertexArray(name_);
}

void VArray::enableVertexAttrib(GLuint index) {
	bind();
	glEnableVertexAttribArray(index);
}

void VArray::disableVertexAttrib(GLuint index) {
	bind();
	glDisableVertexAttribArray(index);
}

void VArray::vertexAttribPointer(GLuint index, GLint size, GLenum type,
                                   GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
	bind();
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void VArray::vertexAttribPointer(GLuint index, GLint size, GLenum type) {
	vertexAttribPointer(index, size, type, GL_FALSE, 0, (void*)0);
}

}
