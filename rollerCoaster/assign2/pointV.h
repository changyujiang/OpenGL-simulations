#include <GL/glu.h>

#ifndef POINTV_H
#define POINTV_H

/* struct of vector with 3 components */
struct pointV {
	GLfloat x;
	GLfloat y;
	GLfloat z;

	pointV(GLfloat X, GLfloat Y, GLfloat Z) : x(X), y(Y), z(Z) {}
	pointV() : x(0), y(0), z(0) {}

	void normalize()
	{
		GLfloat length = x * x + y * y + z * z;
		length = sqrt(length);
		if (length > 0)
		{
			x = x / length;
			y = y / length;
			z = z / length;
		}
	}

	pointV operator+(const pointV& rhs)
	{
		return pointV(this->x+rhs.x, this->y+rhs.y, this->z+rhs.z);
	}

	pointV operator-(const pointV& rhs)
	{
		return pointV(this->x-rhs.x, this->y-rhs.y, this->z-rhs.z);
	}

	pointV operator-()
	{
		return pointV(-this->x, -this->y, -this->z);
	}

	pointV operator*(GLfloat rhs)
	{
		return pointV(this->x*rhs, this->y*rhs, this->z*rhs);
	}
};

#endif