#pragma once

#include <GL/glew.h>

namespace gl {

class VArray {
public:
	VArray();
	VArray(const VArray& other) = delete;
	VArray& operator=(const VArray& other) = delete;
	VArray(VArray&& other);
	VArray& operator=(VArray&& other);
	~VArray();

	void bind() const;

	void enableVertexAttrib(GLuint index);
	void disableVertexAttrib(GLuint index);

	void vertexAttribPointer(GLuint index, GLint size, GLenum type,
			         GLboolean normalized, GLsizei stride, const GLvoid* pointer);
	void vertexAttribPointer(GLuint index, GLint size, GLenum type);

private:
	GLuint name_;
};

}
