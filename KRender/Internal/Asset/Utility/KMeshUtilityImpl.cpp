#include "KMeshUtilityImpl.h"
#include "Internal/Asset/KSubMesh.h"
#include "Internal/KVertexDefinition.h"
#include "Internal/KRenderGlobal.h"

constexpr static float PI = 3.141592654f;
constexpr static float HALF_PI = PI * 0.5f;
constexpr static float TWO_PI = PI * 2.0f;

void KMeshUtilityImpl::PouplateData(const KMeshBoxInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	vertices =
	{
		info.halfExtend * glm::vec3(-1.0f, -1.0, -1.0f),
		info.halfExtend * glm::vec3(-1.0, 1.0f, -1.0f),
		info.halfExtend * glm::vec3(1.0f, 1.0f, -1.0f),
		info.halfExtend * glm::vec3(1.0f, -1.0f, -1.0f),

		info.halfExtend * glm::vec3(1.0f, 1.0f, 1.0f),
		info.halfExtend * glm::vec3(-1.0, 1.0f, 1.0f),
		info.halfExtend * glm::vec3(-1.0, -1.0f, 1.0f),
		info.halfExtend * glm::vec3(1.0f, -1.0f, 1.0f)
	};

	indices =
	{
		0, 1,
		2, 3,
		1, 2,
		3, 0,
		1, 5,
		4, 2,
		0, 6,
		7, 3,
		5, 6,
		7, 4,
		5, 4,
		7, 6
	};

	primtive = DEBUG_PRIMITIVE_LINE;
}

void KMeshUtilityImpl::PouplateData(const KMeshTriangleInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	glm::vec3 normAxisU = glm::normalize(info.axisU);
	glm::vec3 normAxisV = glm::normalize(info.axisV);

	vertices =
	{
		info.origin,
		info.origin + glm::vec3(normAxisU * info.lengthU),
		info.origin + glm::vec3(normAxisV * info.lengthV),
	};

	indices = {};

	primtive = DEBUG_PRIMITIVE_TRIANGLE;
}

void KMeshUtilityImpl::PouplateData(const KMeshCubeInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	vertices =
	{
		info.halfExtend * glm::vec3(-1.0f, -1.0, -1.0f),
		info.halfExtend * glm::vec3(-1.0, 1.0f, -1.0f),
		info.halfExtend * glm::vec3(1.0f, 1.0f, -1.0f),
		info.halfExtend * glm::vec3(1.0f, -1.0f, -1.0f),

		info.halfExtend * glm::vec3(1.0f, 1.0f, 1.0f),
		info.halfExtend * glm::vec3(-1.0, 1.0f, 1.0f),
		info.halfExtend * glm::vec3(-1.0, -1.0f, 1.0f),
		info.halfExtend * glm::vec3(1.0f, -1.0f, 1.0f)
	};

	for (auto& pos : vertices)
	{
		glm::vec4 t = info.transform * glm::vec4(pos, 1.0);
		pos = glm::vec3(t.x, t.y, t.z);
	}

	indices =
	{
		// back
		1, 2, 0, 3, 0, 2,
		// front
		4, 5, 6, 7, 4, 6,
		// left
		5, 1, 0, 6, 5, 0,
		// right
		2, 4, 7, 3, 2, 7,
		// up
		2, 1, 5, 5, 4, 2,
		// down
		0, 3, 6, 7, 6, 3
	};

	primtive = DEBUG_PRIMITIVE_TRIANGLE;
}

void KMeshUtilityImpl::PouplateData(const KMeshQuadInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	glm::vec3 normAxisU = glm::normalize(info.axisU);
	glm::vec3 normAxisV = glm::normalize(info.axisV);

	vertices =
	{
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(normAxisU * info.lengthU),
		glm::vec3(normAxisV * info.lengthV),
		glm::vec3(normAxisU * info.lengthU + normAxisV * info.lengthV),
	};

	for (auto& pos : vertices)
	{
		glm::vec4 t = info.transform * glm::vec4(pos, 1.0);
		pos = glm::vec3(t.x, t.y, t.z);
	}

	indices =
	{
		0, 1, 2,
		3, 2, 1,
	};

	primtive = DEBUG_PRIMITIVE_TRIANGLE;
}

void KMeshUtilityImpl::PouplateData(const KMeshConeInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	const auto sgements = 50;

	vertices.clear();
	vertices.reserve(sgements * 3);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * TWO_PI) * info.radius;
		float bz = sin((float)i / (float)sgements * TWO_PI) * info.radius;

		float cx = cos((float)(i + 1) / (float)sgements * TWO_PI) * info.radius;
		float cz = sin((float)(i + 1) / (float)sgements * TWO_PI) * info.radius;

		vertices.push_back(glm::vec3(bx, 0, bz));
		vertices.push_back(glm::vec3(cx, 0, cz));
		vertices.push_back(glm::vec3(0, info.height, 0));
	}

	for (auto& pos : vertices)
	{
		glm::vec4 t = info.transform * glm::vec4(pos, 1.0);
		pos = glm::vec3(t.x, t.y, t.z);
	}

	indices = {};

	primtive = DEBUG_PRIMITIVE_TRIANGLE;
}

void KMeshUtilityImpl::PouplateData(const KMeshCylinderInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	const auto sgements = 50;

	vertices.clear();
	vertices.reserve(sgements * 6);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * TWO_PI) * info.radius;
		float bz = sin((float)i / (float)sgements * TWO_PI) * info.radius;

		float cx = cos((float)(i + 1) / (float)sgements * TWO_PI) * info.radius;
		float cz = sin((float)(i + 1) / (float)sgements * TWO_PI) * info.radius;

		vertices.push_back(glm::vec3(bx, 0, bz));
		vertices.push_back(glm::vec3(cx, 0, cz));
		vertices.push_back(glm::vec3(bx, info.height, bz));

		vertices.push_back(glm::vec3(cx, info.height, cz));
		vertices.push_back(glm::vec3(bx, info.height, bz));
		vertices.push_back(glm::vec3(cx, 0, cz));
	}

	for (auto& pos : vertices)
	{
		glm::vec4 t = info.transform * glm::vec4(pos, 1.0);
		pos = glm::vec3(t.x, t.y, t.z);
	}

	indices = {};

	primtive = DEBUG_PRIMITIVE_TRIANGLE;
}

void KMeshUtilityImpl::PouplateData(const KMeshCircleInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	const auto sgements = 50;

	vertices.clear();
	vertices.reserve(sgements * 2);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * TWO_PI) * info.radius;
		float bz = sin((float)i / (float)sgements * TWO_PI) * info.radius;

		float cx = cos((float)(i + 1) / (float)sgements * TWO_PI) * info.radius;
		float cz = sin((float)(i + 1) / (float)sgements * TWO_PI) * info.radius;

		vertices.push_back(glm::vec3(bx, 0, bz));
		vertices.push_back(glm::vec3(cx, 0, cz));
	}

	for (auto& pos : vertices)
	{
		glm::vec4 t = info.transform * glm::vec4(pos, 1.0);
		pos = glm::vec3(t.x, t.y, t.z);
	}

	indices = {};

	primtive = DEBUG_PRIMITIVE_LINE;
}

void KMeshUtilityImpl::PouplateData(const KMeshSphereInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	const auto sgements = 50;

	vertices.clear();
	vertices.reserve(sgements * sgements * 6);

	float step_y = PI / sgements;
	float step_xz = TWO_PI / sgements;
	float x[4], y[4], z[4];

	float angle_y = 0.0F;
	float angle_xz = 0.0;

	for (auto i = 0; i < sgements; ++i)
	{
		angle_y = i * step_y - HALF_PI;

		for (auto j = 0; j < sgements; ++j)
		{
			angle_xz = j * step_xz;

			x[0] = info.radius * cos(angle_y) * cos(angle_xz);
			y[0] = info.radius * sin(angle_y);
			z[0] = info.radius * cos(angle_y) * sin(angle_xz);

			x[1] = info.radius * cos(angle_y + step_y) * cos(angle_xz);
			y[1] = info.radius * sin(angle_y + step_y);
			z[1] = info.radius * cos(angle_y + step_y) * sin(angle_xz);

			x[2] = info.radius* cos(angle_y + step_y) * cos(angle_xz + step_xz);
			y[2] = info.radius* sin(angle_y + step_y);
			z[2] = info.radius* cos(angle_y + step_y) * sin(angle_xz + step_xz);

			x[3] = info.radius * cos(angle_y) * cos(angle_xz + step_xz);
			y[3] = info.radius * sin(angle_y);
			z[3] = info.radius * cos(angle_y) * sin(angle_xz + step_xz);

			vertices.push_back(glm::vec3(x[0], y[0], z[0]));
			vertices.push_back(glm::vec3(x[1], y[1], z[1]));
			vertices.push_back(glm::vec3(x[2], y[2], z[2]));

			vertices.push_back(glm::vec3(x[0], y[0], z[0]));
			vertices.push_back(glm::vec3(x[2], y[2], z[2]));
			vertices.push_back(glm::vec3(x[3], y[3], z[3]));
		}
	}

	for (auto& pos : vertices)
	{
		glm::vec4 t = info.transform * glm::vec4(pos, 1.0);
		pos = glm::vec3(t.x, t.y, t.z);
	}

	indices = {};

	primtive = DEBUG_PRIMITIVE_TRIANGLE;
}

void KMeshUtilityImpl::PouplateData(const KMeshArcInfo& info, std::vector<glm::vec3>& vertices, std::vector<uint16_t>& indices, DebugPrimitive& primtive)
{
	const auto sgements = 50;

	vertices.clear();
	vertices.reserve(sgements * 3);

	glm::vec3 xAxis = glm::normalize(info.axis);
	glm::vec3 zAxis = glm::normalize(info.normal);
	glm::vec3 yAxis = glm::cross(zAxis, xAxis);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * info.theta) * info.radius;
		float by = sin((float)i / (float)sgements * info.theta) * info.radius;

		float cx = cos((float)(i + 1) / (float)sgements * info.theta) * info.radius;
		float cy = sin((float)(i + 1) / (float)sgements * info.theta) * info.radius;

		vertices.push_back(bx * xAxis + by * yAxis);
		vertices.push_back(cx * xAxis + cy * yAxis);
		vertices.push_back(glm::vec3(0, 0, 0));
	}

	indices = {};

	primtive = DEBUG_PRIMITIVE_TRIANGLE;
}