#include"stdafx.h"
//#include "SOIL.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#pragma comment(lib, "glfw3.lib")
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include "Shader.h"
#include "GLModel.h"
#include "Line.h"
#include "BezierCurve.h"

enum ProjectionMode
{
	Orthographic, Perspective
};

enum DrawingMode
{
	Polygonal, Bezier, Rotation
};

glm::vec3 cameraPos;
glm::vec3 cameraTarget;
glm::vec3 cameraUp;
glm::vec3 lightPos;

Shader* lineShader;
Shader* linePointsShader;
Shader* rotationBodyShader;
Shader* lampShader;

Line* line;
BezierCurve* bezierCurve;

int windowWidth;
int windowHeight;

const int linesCount = 100;

ProjectionMode projMode = Orthographic;
DrawingMode drawMode = Polygonal;

bool needRecreateModel = true;
bool invertNormals = false;
bool showGrid = false;

GLModel* lineModel;
GLModel* curveModel;
GLModel* rotationBodyModel;
GLModel* controlPointsModel;
GLModel* lampModel;

void draw();
void cleanup();
void recreateModel();
GLModel* createLampModel();
void drawLamp(glm::mat4 vp);

//------------------ГЛ модель

//Конструктор ГЛ модели для 3д
GLModel::GLModel(GLuint vbo, GLuint vao, GLsizei arrayCount, GLenum drawMode)
{
	DrawMode = drawMode;
	VBO = vbo;
	IBO = 0;
	VAO = vao;
	IndexCount = 0;
	ArrayCount = arrayCount;
}

//Конструктор ГЛ модели для линии
GLModel::GLModel(GLint vbo, GLuint ibo, GLuint vao, GLsizei indexCount, GLenum drawMode)
{
	DrawMode = drawMode;
	VBO = vbo;
	IBO = ibo;
	VAO = vao;
	IndexCount = indexCount;
	ArrayCount = 0;
}

//Рисоване ГЛ модели
void GLModel::Draw(GLenum indexType)
{
	glBindVertexArray(VAO);
	if (IBO)
		glDrawElements(DrawMode, IndexCount, indexType, 0);
	else
		glDrawArrays(DrawMode, 0, ArrayCount);
	glBindVertexArray(0);
}

//Диструктор ГЛ модели
GLModel::~GLModel()
{
	if (this->VBO != 0)
		glDeleteBuffers(1, &this->VBO);
	if (this->IBO != 0)
		glDeleteBuffers(1, &this->IBO);
	if (this->VAO != 0)
		glDeleteVertexArrays(1, &this->VAO);
}

//----------------------Основная прога

//Реакция на клавиши клавы 
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	//Показ полигонов
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		showGrid = !showGrid;
		glPolygonMode(GL_FRONT_AND_BACK, showGrid ? GL_LINE : GL_FILL);
	}
	if (key == GLFW_KEY_N && action == GLFW_PRESS)
	{
		invertNormals = !invertNormals;
		needRecreateModel = true;
	}
	//Перспектива/2D
	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
	{
		projMode = (ProjectionMode)((projMode + 1) % 2);
		if (projMode == Perspective && drawMode == Polygonal)
		{
			drawMode = Bezier;
			needRecreateModel = true;
		}
		if (projMode == Orthographic && drawMode == Rotation)
		{
			drawMode = Polygonal;
			needRecreateModel = true;
		}
	}
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		drawMode = (DrawingMode)((drawMode + 1) % 3);
		if (projMode == Orthographic && drawMode == Rotation)
			drawMode = Polygonal;
		if (projMode == Perspective && drawMode == Polygonal)
			drawMode = Bezier;
		needRecreateModel = true;
	}
}
//Реакция на действия мышью
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (projMode == Orthographic && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		double x = xpos;
		double y = windowHeight - ypos;
		cout << "Cursor Position at (" << x << " : " << y << ")" << endl;
		line->AddPoint(x, y);
		needRecreateModel = true;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		line->DeleleLastPoint();
		needRecreateModel = true;
	}
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "l4", nullptr, nullptr);
	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
	glViewport(0, 0, windowWidth, windowHeight);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	lineShader = new Shader("lineShader.vert", "lineShader.frag");
	linePointsShader = new Shader("linePoints.vert", "linePoints.frag");
	rotationBodyShader = new Shader("rotationBody.vert", "rotationBody.frag");
	lampShader = new Shader("lampShader.vert", "lampShader.frag");

	cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
	cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
	lightPos = glm::vec3(windowWidth * 1.0f, windowHeight * 0.0f, windowHeight * 1.0f);

	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 cameraDirection = glm::normalize(cameraTarget - cameraPos);
	glm::vec3 cameraFront = glm::normalize(glm::cross(up, cameraDirection));
	cameraUp = glm::cross(cameraDirection, cameraFront);

	glEnable(GL_DEPTH_TEST);

	line = new Line();
	lampModel = createLampModel();

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		draw();
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	cleanup();
	return 0;
}

void draw()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glm::mat4 model, view, projection;

	if (projMode == Orthographic)
	{
		cameraPos = glm::vec3(0.0f, 0.0f, 1.0f);
		view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
		projection = glm::ortho(0.0f, (float)windowWidth, 0.0f, (float)windowHeight, 0.0f, 1.1f);
	}
	else if (projMode == Perspective)
	{
		cameraPos = glm::vec3(1.75f * (float)windowWidth, 1.2f * (float)windowHeight, 1.74f * (float)windowHeight);
		model = glm::rotate(model, (float)glm::radians(glfwGetTime() * 50.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		view = glm::lookAt(cameraPos, cameraTarget, cameraUp);
		projection = glm::perspective(glm::radians(43.0f), (float)windowWidth / windowHeight, 0.1f, (float)windowWidth * 15.0f);
	}

	glm::mat4 vpMatrix = projection * view;
	glm::mat4 mvpMatrix = vpMatrix * model;

	lineShader->Use();
	GLint mvpMatrixLoc = glGetUniformLocation(lineShader->Program, "mvpMatrix");
	glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

	linePointsShader->Use();
	mvpMatrixLoc = glGetUniformLocation(linePointsShader->Program, "mvpMatrix");
	glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

	rotationBodyShader->Use();
	mvpMatrixLoc = glGetUniformLocation(rotationBodyShader->Program, "mvpMatrix");
	glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));

	linePointsShader->Use();
	GLint pointColorLoc = glGetUniformLocation(linePointsShader->Program, "pointColor");
	glUniform3f(pointColorLoc, 0.0f, 0.0f, 0.0f);
	GLModel* vertexModel = line->CreateVertexModel();
	vertexModel->Draw();
	delete vertexModel;

	recreateModel();

	if (drawMode == Bezier)
	{
		linePointsShader->Use();
		GLint pointColorLoc = glGetUniformLocation(linePointsShader->Program, "pointColor");
		glUniform3f(pointColorLoc, 1.0f, 0.0f, 0.0f);
		controlPointsModel->Draw();

		lineShader->Use();
		curveModel->Draw();
	}
	else if (drawMode == Polygonal)
	{
		lineShader->Use();
		lineModel->Draw();
	}
	else if (drawMode == Rotation)
	{
		rotationBodyShader->Use();

		glm::mat3 normalMatrix;
		normalMatrix = (glm::transpose(glm::inverse(glm::mat3(model))));

		GLint mMatrixLoc = glGetUniformLocation(rotationBodyShader->Program, "mMatrix");
		GLint mvpMatrixLoc = glGetUniformLocation(rotationBodyShader->Program, "mvpMatrix");
		GLint normalMatrixLoc = glGetUniformLocation(rotationBodyShader->Program, "normalMatrix");

		glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, glm::value_ptr(model));
		glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
		glUniformMatrix3fv(normalMatrixLoc, 1, GL_FALSE, glm::value_ptr(normalMatrix));

		GLint objectColorLoc = glGetUniformLocation(rotationBodyShader->Program, "objectColor");
		GLint lightColorLoc = glGetUniformLocation(rotationBodyShader->Program, "lightColor");
		GLint viewPositionLoc = glGetUniformLocation(rotationBodyShader->Program, "viewPosition");
		GLint lightPositionLoc = glGetUniformLocation(rotationBodyShader->Program, "lightPosition");

		glUniform3f(objectColorLoc, 1.0f, 0.2f, 0.83f);
		glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
		glUniform3f(viewPositionLoc, cameraPos.x, cameraPos.y, cameraPos.z);
		glUniform3f(lightPositionLoc, lightPos.x, lightPos.y, lightPos.z);

		rotationBodyModel->Draw();

		lampShader->Use();
		drawLamp(vpMatrix);
	}
}

//Глобальная очистка
void cleanup()
{
	if (lineShader) delete lineShader;
	if (linePointsShader) delete linePointsShader;
	if (rotationBodyShader) delete rotationBodyShader;
	if (lampShader) delete lampShader;

	if (lineModel) delete lineModel;
	if (curveModel)	delete curveModel;
	if (rotationBodyModel) delete rotationBodyModel;
	if (controlPointsModel)	delete controlPointsModel;
	if (lampModel) delete lampModel;

	if (line) delete line;
	if (bezierCurve) delete bezierCurve;

}

//Перерисовка модели
void recreateModel()
{
	if (!needRecreateModel) return;

	if (drawMode == Bezier)
	{
		if (bezierCurve) delete bezierCurve;
		bezierCurve = new BezierCurve(line->GetPoints());

		if (controlPointsModel) delete controlPointsModel;
		controlPointsModel = bezierCurve->CreateControlPointsModel();

		if (curveModel) delete curveModel;
		curveModel = bezierCurve->CreateCurveModel();
	}
	else if (drawMode == Polygonal) {
		if (lineModel) delete lineModel;
		lineModel = line->CreateLineModel();
	}
	else {
		if (bezierCurve) delete bezierCurve;
		bezierCurve = new BezierCurve(line->GetPoints());

		if (rotationBodyModel) delete rotationBodyModel;
		rotationBodyModel = bezierCurve->CreateRotationBody(linesCount, invertNormals);
	}

	needRecreateModel = false;
}

//Создание модели освещения
GLModel* createLampModel()
{
	GLuint lampVBO, lampVAO, lampIBO;
	glGenVertexArrays(1, &lampVAO);
	glGenBuffers(1, &lampVBO);
	glGenBuffers(1, &lampIBO);

	glBindVertexArray(lampVAO);

	GLfloat cubeVertices[] =
	{
		// front 
		-1.0, -1.0,  1.0,
		1.0, -1.0,  1.0,
		1.0,  1.0,  1.0,
		-1.0,  1.0,  1.0,
		// back
		-1.0, -1.0, -1.0,
		1.0, -1.0, -1.0,
		1.0,  1.0, -1.0,
		-1.0,  1.0, -1.0,
	};

	GLuint cubeIndeces[] = {
		// front
		0, 1, 2,
		2, 3, 0,
		// top
		1, 5, 6,
		6, 2, 1,
		// back
		7, 6, 5,
		5, 4, 7,
		// bottom
		4, 0, 3,
		3, 7, 4,
		// left
		4, 5, 1,
		1, 0, 4,
		// right
		3, 2, 6,
		6, 7, 3,
	};
	glBindBuffer(GL_ARRAY_BUFFER, lampVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lampIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndeces), cubeIndeces, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	return new GLModel(lampVBO, lampIBO, lampVAO, 36, GL_TRIANGLES);
}

void drawLamp(glm::mat4 vp)
{
	lampShader->Use();

	glm::mat4 model;
	model = glm::translate(model, lightPos);
	model = glm::scale(model, glm::vec3(10.0f));
	glm::mat4 mvp = vp * model;

	GLint mvpMatrixLoc = glGetUniformLocation(lampShader->Program, "mvpMatrix");
	glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, glm::value_ptr(mvp));

	glBindVertexArray(lampModel->VAO);
	glDrawElements(GL_TRIANGLES, lampModel->IndexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

//---------------------Все шейдеры

//Конструктор
Shader::Shader(string pathToVertexShader, string pathToFragmentShader)
{
	//Считывание из файлов в текст
	string vertexCode = readShaderCodeFromFile(pathToVertexShader);
	string fragmentCode = readShaderCodeFromFile(pathToFragmentShader);
	//Создание шейдера по тексту
	GLuint vertexShader = createShader(vertexCode, GL_VERTEX_SHADER);
	GLuint fragmentShader = createShader(fragmentCode, GL_FRAGMENT_SHADER);

	Program = createShaderProgram(vertexShader, fragmentShader);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::Use()
{
	glUseProgram(this->Program);
}

//Диструктор
Shader::~Shader()
{
	cout << "Cleanup shader" << endl;
	if (this->Program != 0)
		glDeleteProgram(this->Program);
}

//Создание шейдера из файла
GLuint Shader::createShader(const string code, GLenum type)
{
	GLuint result = glCreateShader(type);
	const GLchar* glCode = code.c_str();
	glShaderSource(result, 1, &glCode, NULL);
	glCompileShader(result);

	GLint compiled;
	glGetShaderiv(result, GL_COMPILE_STATUS, &compiled);

	if (!compiled)
	{
		GLint infoLen = 0;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 0)
		{
			char* infoLog = new char[infoLen];
			glGetShaderInfoLog(result, infoLen, NULL, infoLog);
			cout << "Shader compilation error" << endl << infoLog << endl;
		}
		glDeleteShader(result);
		return 0;
	}

	return result;
}

//Создание шейдера из фрагментного и вершинного
GLuint Shader::createShaderProgram(GLuint vertexShader, GLuint fragmentShader)
{
	GLuint program = glCreateProgram();

	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	GLint linked;
	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if (!linked)
	{
		GLint infoLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 0)
		{
			char* infoLog = new char[infoLen];
			glGetProgramInfoLog(program, infoLen, NULL, infoLog);
			cout << "Shader program linking error" << endl << infoLog << endl;
		}
		glDeleteProgram(program);
		return 0;
	}

	return program;
}

string Shader::readShaderCodeFromFile(string pathToFile)
{
	string vertexCode;
	ifstream vShaderFile;
	vShaderFile.exceptions(ifstream::badbit | ifstream::failbit);
	try
	{
		vShaderFile.open(pathToFile);
		stringstream vShaderStream;
		vShaderStream << vShaderFile.rdbuf();
		vShaderFile.close();
		vertexCode = vShaderStream.str();
	}
	catch (ifstream::failure e)
	{
		cout << "Error reading the file" << endl;
		throw e;
	}
	return vertexCode;
}

//-------------------Рисовка линий

Line::Line()
{
}

//Добавка точки x,y к линии
void Line::AddPoint(GLfloat x, GLfloat y)
{
	Vertices.push_back(x);
	Vertices.push_back(y);
}

//Тоочки через поинт
void Line::AddPoint(Point2D point)
{
	AddPoint(point.X, point.Y);
}

//Удаление последней точки
void Line::DeleleLastPoint()
{
	if (Vertices.size() > 0)
	{
		Vertices.pop_back();
		Vertices.pop_back();
	}
}

//Кол-во точек, по которым строится прямая
GLsizei Line::Count()
{
	return Vertices.size() / 2;
}

//Создание линии
GLModel* Line::CreateLineModel()
{
	GLuint lineVBO, lineVAO;
	glGenVertexArrays(1, &lineVAO);
	glGenBuffers(1, &lineVBO);

	glBindVertexArray(lineVAO);

	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);

	if (!Vertices.empty())
		glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(GLfloat), &Vertices.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	return new GLModel(lineVBO, lineVAO, Count(), GL_LINE_STRIP);
}

//Создание вершинной модели линии (Пряямоугольник из 2х треугольников)
GLModel * Line::CreateVertexModel()
{
	GLuint vertexVBO, vertexVAO, vertexIBO;
	glGenVertexArrays(1, &vertexVAO);
	glGenBuffers(1, &vertexVBO);
	glGenBuffers(1, &vertexIBO);

	glBindVertexArray(vertexVAO);

	int count = this->Count();
	vector<GLfloat> vertices;
	vector<GLuint> indices;

	if (count > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
		const float VertexWidth = 5.0f;
		GLfloat offsets[2] = { -VertexWidth, VertexWidth };
		GLuint rectangleIndices[6] = { 0, 1, 3, 0, 3, 2 };
		for (int i = 0; i < count; i++)
		{
			Point2D currentVertex = this->GetPoint(i);
			for (int j = 0; j < 2; j++)
				for (int k = 0; k < 2; k++)
				{
					vertices.push_back(currentVertex.X + offsets[j]);
					vertices.push_back(currentVertex.Y + offsets[k]);
				}
			for (int j = 0; j < 6; j++)
				indices.push_back(4 * i + rectangleIndices[j]);
		}
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices.front(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices.front(), GL_STATIC_DRAW);
	}

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	return new GLModel(vertexVBO, vertexIBO, vertexVAO, indices.size(), GL_TRIANGLES);
}

//Получение точки
Point2D Line::GetPoint(int index)
{
	return Point2D(Vertices[2 * index], Vertices[2 * index + 1]);
}

//Получение вектора точек
vector<Point2D> Line::GetPoints()
{
	vector<Point2D> points;
	for (int i = 0; i < Count(); i++)
		points.push_back(GetPoint(i));
	return points;
}

//-------------------Кривая бизье

//Создание кривой
GLModel * BezierCurve::CreateCurveModel()
{
	if (Vertices.empty() || Normals.empty())
		return new GLModel(0, 0, 0, 0);

	GLuint verticesVBO, normalsVBO, lineVAO;
	glGenVertexArrays(1, &lineVAO);
	glGenBuffers(1, &verticesVBO);
	glGenBuffers(1, &normalsVBO);
	glBindVertexArray(lineVAO);

	glBindBuffer(GL_ARRAY_BUFFER, verticesVBO);
	glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(GLfloat), &Vertices[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, normalsVBO);
	glBufferData(GL_ARRAY_BUFFER, Normals.size() * sizeof(GLfloat), &Normals[0], GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	return new GLModel(verticesVBO, lineVAO, Count(), GL_LINE_STRIP);
}

//Создание контрольных точек для кривой
GLModel * BezierCurve::CreateControlPointsModel()
{
	if (Vertices.empty() || Normals.empty())
		return new GLModel(0, 0, 0, 0);

	GLuint vertexVBO, vertexVAO, vertexIBO;
	glGenVertexArrays(1, &vertexVAO);
	glGenBuffers(1, &vertexVBO);
	glGenBuffers(1, &vertexIBO);

	glBindVertexArray(vertexVAO);

	int count = ControlPoints.size();

	vector<GLfloat> vertices;
	vector<GLuint> indices;

	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	const float VertexWidth = 5.0f;
	GLfloat offsets[2] = { -VertexWidth, VertexWidth };
	GLuint rectangleIndices[6] = { 0, 1, 3, 0, 3, 2 };
	for (int i = 0; i < count; i += 2)
	{
		Point2D currentCP = Point2D(ControlPoints[i], ControlPoints[i + 1]);
		for (int j = 0; j < 2; j++)
			for (int k = 0; k < 2; k++)
			{
				vertices.push_back(currentCP.X + offsets[j]);
				vertices.push_back(currentCP.Y + offsets[k]);
			}
		for (int j = 0; j < 6; j++)
			indices.push_back(2 * i + rectangleIndices[j]);
	}
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices.front(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	return new GLModel(vertexVBO, vertexIBO, vertexVAO, indices.size(), GL_TRIANGLES);
}

//Создание тела врщения
GLModel * BezierCurve::CreateRotationBody(GLsizei linesCount, bool invertNormals)
{
	if (Vertices.empty() || Normals.empty())
		return new GLModel(0, 0, 0, 0);

	cout << "ver count = " << Vertices.size() << endl;
	cout << "norm count = " << Normals.size() << endl;
	const GLfloat degrees_per_rotation = 360.0f / linesCount;
	vector<GLfloat> vertices;
	int m = invertNormals ? -1 : 1;
	for (int i = 0; i < linesCount; i++)
	{
		for (int j = 0; j < Vertices.size(); j += 2)
		{
			glm::mat4 transform = glm::mat4(1.0f);
			glm::vec2 currentPos = glm::vec2(Vertices[j], Vertices[j + 1]);
			glm::vec2 norm = glm::vec2(Normals[j] * m, Normals[j + 1] * m);
			transform = glm::translate(transform, glm::vec3(currentPos, 0.0f));
			transform = glm::rotate(transform, glm::radians(i * degrees_per_rotation), glm::vec3(1.0f, 0.0f, 0.0f));
			transform = glm::translate(transform, glm::vec3(0.0f, 0.0f, 0.0f));
			const glm::vec3 vertex = glm::vec3(glm::vec4(currentPos, 0.0f, 0.0f) * transform);
			const glm::vec3 normal = glm::vec3(glm::vec4(norm, 0.0f, 0.0f) * transform);

			vertices.push_back(vertex.x);
			vertices.push_back(vertex.y);
			vertices.push_back(vertex.z);

			vertices.push_back(normal.x);
			vertices.push_back(normal.y);
			vertices.push_back(normal.z);
		}
	}
	const GLuint line_size = Count();
	vector<GLuint> indices;

	for (int i = 0; i < linesCount; i++)
	{
		for (int j = 0; j < line_size - 1; j++)
		{
			indices.push_back(j + i * line_size);
			indices.push_back(j + 1 + i * line_size);
			indices.push_back((j + 1 + line_size + i * line_size) % (line_size * linesCount));
			indices.push_back(j + i * line_size);
			indices.push_back((j + line_size + i * line_size) % (line_size * linesCount));
			indices.push_back((j + 1 + line_size + i * line_size) % (line_size * linesCount));
		}
	}

	GLuint vertexVBO, vertexVAO, vertexIBO;
	glGenVertexArrays(1, &vertexVAO);
	glGenBuffers(1, &vertexVBO);
	glGenBuffers(1, &vertexIBO);

	glBindVertexArray(vertexVAO);

	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), &vertices.front(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vertexIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), &indices.front(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);

	return new GLModel(vertexVBO, vertexIBO, vertexVAO, indices.size(), GL_TRIANGLES);
}

//Кол-во вершин в кривой
GLuint BezierCurve::Count()
{
	return Vertices.size() / 2;
}

//Главная функия
BezierCurve::BezierCurve(vector<Point2D> points)
{
	PlotBezierCurve(points);
}

//Рисование кривой бизье
void BezierCurve::PlotBezierCurve(vector<Point2D> points)
{
	int count = points.size();

	if (count == 0) return;

	for (int i = 0; i < count - 3; i += 3)
	{
		Point2D p3 = points[i + 3];
		Point2D p2 = points[i + 2];
		Point2D p1 = points[i + 1];
		Point2D p0 = points[i + 0];

		AddCurve(p0, p1, p2, p3, false);//true
	}

	//Для проверки, есть ли вообще точки, вычитаем 1
	int pointsLeft = (count - 1) % 3; // кол-во оставшихся точек

	if (pointsLeft > 0)
	{
		
		//p0, p1 - опорные ; p2,p3 - управляющие
		Point2D p0 = points[count - pointsLeft - 1];
		Point2D p1 = points[count - pointsLeft];
		Point2D p2 = points[count - 1];
		Point2D p3 = Point2D(p2.X, p2.Y);
		if (pointsLeft == 2)
			p2 = points[count - pointsLeft + 1];

		/*if (pointsLeft == 1 && count > 3)
		{
			p1 = points[count - 3];
			p2 = points[count - 2];
		}*/
		AddCurve(p0, p1, p2, p3, false);
	}

	CalculateNormals();
}

//
std::tuple<Point2D, Point2D> BezierCurve::ComputeControlPoints(Point2D d0, Point2D d1, Point2D d2, Point2D d3)
{
	Point2D p0 = d0;
	Point2D p3 = d3;

	float t11 = t1;
	float t12 = t1 * t1;
	float t13 = t11 * t12;
	float u11 = 1 - t1;
	float u12 = u11 * u11;
	float u13 = u11 * u12;
	float t21 = t2;
	float t22 = t2 * t2;
	float t23 = t21 * t22;
	float u21 = 1 - t2;
	float u22 = u21 * u21;
	float u23 = u21 * u22;

	GLfloat p1x = t21 * (d1.X - u13 * p0.X - t13 * p3.X) / (3 * u11 * t11 * (t21 - t11)) -
		t11 * (d2.X - u23 * p0.X - t23 * p3.X) / (3 * u21 * t21 * (t21 - t11));

	GLfloat p1y = t21 * (d1.Y - u13 * p0.Y - t13 * p3.Y) / (3 * u11 * t11 * (t21 - t11)) -
		t11 * (d2.Y - u23 * p0.Y - t23 * p3.Y) / (3 * u21 * t21 * (t21 - t11));

	Point2D p1 = Point2D(p1x, p1y);

	GLfloat p2x = (d1.X - u13 * p0.X - t13 * p3.X) / (3 * u11 * t12) - u11 * p1.X / t11;
	GLfloat p2y = (d1.Y - u13 * p0.Y - t13 * p3.Y) / (3 * u11 * t12) - u11 * p1.Y / t11;

	Point2D p2 = Point2D(p2x, p2y);

	return std::make_tuple(p1, p2);
}

//Добавить крувую
void BezierCurve::AddCurve(Point2D d0, Point2D d1, Point2D d2, Point2D d3, bool throughControlPoint)
{
	Point2D p0 = d0;
	Point2D p1 = d1;
	Point2D p2 = d2;
	Point2D p3 = d3;

	if (throughControlPoint)
	{
		tuple<Point2D, Point2D> controlPoints = ComputeControlPoints(d0, d1, d2, d3);
		p1 = std::get<0>(controlPoints);
		p2 = std::get<1>(controlPoints);
	}

	ControlPoints.push_back(p1.X);
	ControlPoints.push_back(p1.Y);
	ControlPoints.push_back(p2.X);
	ControlPoints.push_back(p2.Y);

	for (GLfloat t = 0; t <= 1.0f; t += 0.01f)
	{
		float t1 = t;
		float t2 = t * t;
		float t3 = t1 * t2;
		float u1 = 1 - t;
		float u2 = u1 * u1;
		float u3 = u1 * u2;

		GLfloat x = u3 * p0.X + 3 * u2 * t1 * p1.X + 3 * u1 * t2 * p2.X + t3 * p3.X;
		GLfloat y = u3 * p0.Y + 3 * u2 * t1 * p1.Y + 3 * u1 * t2 * p2.Y + t3 * p3.Y;

		Vertices.push_back(x);
		Vertices.push_back(y);
	}
}

//Вычисление нормалей
void BezierCurve::CalculateNormals()
{
	if (Vertices.empty()) return;
	GLfloat px = Vertices[0];
	GLfloat py = Vertices[1];

	bool first = true;
	for (int i = 2; i < Vertices.size() - 2; i += 2)
	{
		GLfloat x = Vertices[i];
		GLfloat y = Vertices[i + 1];
		GLfloat nx = Vertices[i + 2];
		GLfloat ny = Vertices[i + 3];
		glm::vec2 nxy = glm::normalize(glm::vec2(nx - px, ny - py));
		Normals.push_back(-nxy.y);
		Normals.push_back(nxy.x);
		if (first)
		{
			Normals.push_back(-nxy.y);
			Normals.push_back(nxy.x);
			first = false;
		}
		px = x;
		py = y;
	}

	int count = Normals.size();
	Normals.push_back(Normals[count - 2]);
	Normals.push_back(Normals[count - 1]);
}

