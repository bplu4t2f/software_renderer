#pragma once

#include <vector>

struct Vec3f
{
	Vec3f()
	{
	}

	Vec3f(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	float x, y, z;
};

struct Face
{
	Face()
	{
	}

	Face(int v0, int v1, int v2)
	{
		this->vertices[0] = v0;
		this->vertices[1] = v1;
		this->vertices[2] = v2;
	}

	int vertices[3];
};

struct Model
{
	std::vector<Vec3f> vertices;
	std::vector<Face> faces;
};
