
#include "VArray.hpp"
#include <GL/glew.h>
#include <utility>

namespace gl {

//opengl state shadowing for performance
static thread_local GLuint curVAO = 0;
static void vaoBind(GLuint name) {
	if(curVAO != name) {
		glBindVertexArray(name);
		curVAO = name;
	}
}

VArray::VArray() : name_(0)
{
	if(GLEW_ARB_direct_state_access) {
		glCreateVertexArrays(1, &name_);
	} else {
		glGenVertexArrays(1, &name_);
	}
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
	if (curVAO == name_) {
		curVAO = 0;
	}
	glDeleteVertexArrays(1, &name_);
}

void VArray::bind() const {
	vaoBind(name_);
}

void VArray::enableVertexAttrib(GLuint index) {
	if(GLEW_ARB_direct_state_access) {
		glEnableVertexArrayAttrib(name_, index);
		return;
	}

	vaoBind(name_);
	glEnableVertexAttribArray(index);
}

void VArray::disableVertexAttrib(GLuint index) {
	if(GLEW_ARB_direct_state_access) {
		glDisableVertexArrayAttrib(name_, index);
		return;
	}

	vaoBind(name_);
	glDisableVertexAttribArray(index);
}

void VArray::vertexAttribPointer(GLuint index, GLint size, GLenum type,
                                   GLboolean normalized, GLsizei stride, const GLvoid* pointer) {
	vaoBind(name_);
	glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void VArray::vertexAttribPointer(GLuint index, GLint size, GLenum type) {
	vertexAttribPointer(index, size, type, GL_FALSE, 0, (void*)0);
}

}
