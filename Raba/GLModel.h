#ifndef GLMODEL_H
#define GLMODEL_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Shader.h"

class GLModel
{
public:
	GLuint VBO;
	GLuint IBO;
	GLuint VAO;

	GLsizei ArrayCount;
	GLsizei IndexCount;

	GLenum DrawMode;

	GLModel(GLuint vbo, GLuint vao, GLsizei arrayCount, GLenum drawMode);
	GLModel(GLint vbo, GLuint ibo, GLuint vao, GLsizei indexCount, GLenum drawMode);

	void Draw(GLenum indexType = GL_UNSIGNED_INT);

	~GLModel();
};

#endif