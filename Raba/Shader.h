#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <GL/glew.h>; 

using namespace std;

class Shader
{
public:
	GLuint Program;
	Shader(string vertexPath, string fragmentPath);
	void Use();
	~Shader();
private:
	GLuint createShader(const string code, GLenum type);
	GLuint createShaderProgram(GLuint vertexShader, GLuint fragmentShader);
	string readShaderCodeFromFile(string pathToFile);
};

#endif