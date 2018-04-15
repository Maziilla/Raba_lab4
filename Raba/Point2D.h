#ifndef POINT2D_H
#define POINT2D_H

#include <GL/glew.h>

class Point2D
{
public:
	GLfloat X;
	GLfloat Y;

	Point2D operator +(const Point2D &p) const { return Point2D(X + p.X, Y + p.Y); };
    Point2D operator -(const Point2D &p) const { return Point2D(X - p.X, Y - p.Y); };
    Point2D operator *(float c) const { return Point2D(c * X, c * Y); };

	Point2D()
	{
	}

	Point2D(GLfloat x, GLfloat y)
	{
		this->X = x;
		this->Y = y;
	}
};

#endif