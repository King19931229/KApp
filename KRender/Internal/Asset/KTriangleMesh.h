#pragma once
#include "Publish/KTriangle.h"

struct KTriangleMesh
{
	std::vector<KTriangle> triangles;
	void Destroy()
	{
		triangles.clear();
		triangles.shrink_to_fit();
	}
};