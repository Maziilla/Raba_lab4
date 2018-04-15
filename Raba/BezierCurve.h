#ifndef BEZIER_CURVE_H
#define BEZIER_CURVE_H

#include <GL/glew.h>
#include "Line.h"
#include "GLModel.h"
#include <vector>
#include "Point2D.h"
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <tuple>

using namespace std;

// https://stackoverflow.com/questions/20417456/how-to-draw-curve-with-four-points
class BezierCurve
{
public:
	BezierCurve(vector<Point2D> points);

	GLuint Count();
	
	GLModel * CreateCurveModel();
	GLModel * CreateControlPointsModel();
	GLModel * CreateRotationBody(GLsizei linesCount, bool invertNormals);

private:
	vector<GLfloat> Vertices;
	vector<GLfloat> Normals;
	vector<GLfloat> ControlPoints;

	const GLfloat t2 = 2.0f / 3.0f;
	const GLfloat t1 = 1.0f / 3.0f;

	std::tuple<Point2D, Point2D> ComputeControlPoints(Point2D d0, Point2D d1, Point2D d2, Point2D d3);
	void PlotBezierCurve(vector<Point2D> points);
	void AddCurve(Point2D d0, Point2D d1, Point2D d2, Point2D d3, bool throughControlPoint);
	void CalculateNormals();
};

#endif