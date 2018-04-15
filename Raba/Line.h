#ifndef LINE_H
#define LINE_H

#include <GL/glew.h>
#include "GLModel.h"
#include "Point2D.h"
#include <vector>

using namespace std;

class Line
{
public:
	Line();
	void AddPoint(GLfloat x, GLfloat y);
	void AddPoint(Point2D point);
	void DeleleLastPoint();
	Point2D GetPoint(int index);
	vector<Point2D> GetPoints();
	GLsizei Count();
	GLModel* CreateLineModel();
	GLModel* CreateVertexModel();
protected:
	vector<GLfloat> Vertices;
};

#endif