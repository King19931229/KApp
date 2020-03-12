#include "KMeshUtility.h"
#include "Internal/Asset/KSubMesh.h"
#include "Internal/KVertexDefinition.h"
#include <assert.h>

namespace KMeshUtility
{
	bool CreateBox(IKRenderDevice* device, KMesh* pMesh, const glm::vec3& halfExtent, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateBox(pMesh, halfExtent, frameInFlight);
	}

	bool CreateQuad(IKRenderDevice* device, KMesh* pMesh, const glm::mat4& transform,
		float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateQuad(pMesh, transform, lengthU, lengthV, axisU, axisV, frameInFlight);
	}

	bool CreateCone(IKRenderDevice* device, KMesh* pMesh, const glm::mat4& transform,
		float height, float radius, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateCone(pMesh, transform, height, radius, frameInFlight);
	}

	bool CreateCylinder(IKRenderDevice* device, KMesh* pMesh, const glm::mat4& transform,
		float height, float radius, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateCylinder(pMesh, transform, height, radius, frameInFlight);
	}

	bool CreateCircle(IKRenderDevice* device, KMesh* pMesh, const glm::mat4& transform,
		float radius, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateCircle(pMesh, transform, radius, frameInFlight);
	}

	bool CreateSphere(IKRenderDevice* device, KMesh* pMesh, const glm::mat4& transform,
		float radius, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateSphere(pMesh, transform, radius, frameInFlight);
	}

	bool CreateArc(IKRenderDevice* device, KMesh* pMesh,
		const glm::vec3& axis, const glm::vec3& normal, float radius, float theta, size_t frameInFlight)
	{
		KMeshUtilityImpl impl(device);
		return impl.CreateArc(pMesh, axis, normal, radius, theta, frameInFlight);
	}
}

KMeshUtilityImpl::KMeshUtilityImpl(IKRenderDevice* device)
	: m_Device(device)
{
	assert(device);
}

KMeshUtilityImpl::~KMeshUtilityImpl()
{
}

bool KMeshUtilityImpl::CreateBox(KMesh* pMesh, const glm::vec3& halfExtend, size_t frameInFlight)
{
	if (pMesh)
	{
		pMesh->UnInit();

		KVertexData& vertexData = pMesh->m_VertexData;

		IKVertexBufferPtr vertexBuffer = nullptr;
		m_Device->CreateVertexBuffer(vertexBuffer);

		KAABBBox bound;
		bound.InitFromHalfExtent(glm::vec3(0.0f), halfExtend);

		KVertexDefinition::DEBUG_POS_3F positions[] =
		{
			{ halfExtend * glm::vec3(-1.0f, -1.0, -1.0f) },
			{ halfExtend * glm::vec3(-1.0, 1.0f, -1.0f) },
			{ halfExtend * glm::vec3(1.0f, 1.0f, -1.0f) },
			{ halfExtend * glm::vec3(1.0f, -1.0f, -1.0f) },

			{ halfExtend * glm::vec3(1.0f, 1.0f, 1.0f) },
			{ halfExtend * glm::vec3(-1.0, 1.0f, 1.0f) },
			{ halfExtend * glm::vec3(-1.0, -1.0f, 1.0f) },
			{ halfExtend * glm::vec3(1.0f, -1.0f, 1.0f) }
		};

		vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
		vertexData.vertexBuffers.push_back(vertexBuffer);
		vertexData.vertexStart = 0;
		vertexData.vertexCount = ARRAY_SIZE(positions);
		vertexData.bound = bound;

		auto halfExtend = bound.GetExtend() * 0.5f;

		vertexBuffer->InitMemory(ARRAY_SIZE(positions), sizeof(positions[0]), positions);
		vertexBuffer->InitDevice(false);

		uint16_t indices[] =
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
			7, 4
		};

		IKIndexBufferPtr indexBuffer = nullptr;
		m_Device->CreateIndexBuffer(indexBuffer);

		indexBuffer->InitMemory(IT_16, ARRAY_SIZE(indices), indices);
		indexBuffer->InitDevice(false);

		KIndexData indexData;
		indexData.indexBuffer = indexBuffer;
		indexData.indexStart = 0;
		indexData.indexCount = ARRAY_SIZE(indices);

		auto& subMeshes = pMesh->m_SubMeshes;

		KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
		newSubMesh->InitDebug(DEBUG_PRIMITIVE_LINE, &vertexData, &indexData, frameInFlight);

		subMeshes.push_back(newSubMesh);

		return true;
	}
	return false;
}

bool KMeshUtilityImpl::CreateQuad(KMesh* pMesh, const glm::mat4& transform, float lengthU, float lengthV, const glm::vec3& axisU, const glm::vec3& axisV, size_t frameInFlight)
{
	if (pMesh)
	{
		pMesh->UnInit();

		KVertexData& vertexData = pMesh->m_VertexData;

		IKVertexBufferPtr vertexBuffer = nullptr;
		m_Device->CreateVertexBuffer(vertexBuffer);

		glm::vec3 normAxisU = glm::normalize(axisU);
		glm::vec3 normAxisV = glm::normalize(axisV);

		KVertexDefinition::DEBUG_POS_3F positions[] =
		{
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(normAxisU * lengthU),
			glm::vec3(normAxisV * lengthV),
			glm::vec3(normAxisU * lengthU + normAxisV * lengthV),
		};

		KAABBBox bound;

		for (auto& pos : positions)
		{
			glm::vec4 t = transform * glm::vec4(pos.DEBUG_POSITION, 1.0);
			pos.DEBUG_POSITION = glm::vec3(t.x, t.y, t.z);
			bound.Merge(pos.DEBUG_POSITION, bound);
		}

		vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
		vertexData.vertexBuffers.push_back(vertexBuffer);
		vertexData.vertexStart = 0;
		vertexData.vertexCount = ARRAY_SIZE(positions);
		vertexData.bound = bound;

		vertexBuffer->InitMemory(ARRAY_SIZE(positions), sizeof(positions[0]), positions);
		vertexBuffer->InitDevice(false);

		uint16_t indices[] =
		{
			0, 1, 2,
			3, 2, 1,
		};

		IKIndexBufferPtr indexBuffer = nullptr;
		m_Device->CreateIndexBuffer(indexBuffer);

		indexBuffer->InitMemory(IT_16, ARRAY_SIZE(indices), indices);
		indexBuffer->InitDevice(false);

		KIndexData indexData;
		indexData.indexBuffer = indexBuffer;
		indexData.indexStart = 0;
		indexData.indexCount = ARRAY_SIZE(indices);

		auto& subMeshes = pMesh->m_SubMeshes;

		KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
		newSubMesh->InitDebug(DEBUG_PRIMITIVE_TRIANGLE, &vertexData, &indexData, frameInFlight);

		subMeshes.push_back(newSubMesh);

		return true;
	}
	return false;
}

bool KMeshUtilityImpl::CreateCone(KMesh* pMesh, const glm::mat4& transform, float height, float radius, size_t frameInFlight)
{
	const auto sgements = 50;

	std::vector<KVertexDefinition::DEBUG_POS_3F> positions;
	positions.reserve(sgements * 3);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * TWO_PI) * radius;
		float bz = sin((float)i / (float)sgements * TWO_PI) * radius;

		float cx = cos((float)(i + 1) / (float)sgements * TWO_PI) * radius;
		float cz = sin((float)(i + 1) / (float)sgements * TWO_PI) * radius;

		positions.push_back({ glm::vec3(bx, 0, bz) });
		positions.push_back({ glm::vec3(cx, 0, cz) });
		positions.push_back({ glm::vec3(0, height, 0) });
	}

	pMesh->UnInit();

	KVertexData& vertexData = pMesh->m_VertexData;

	IKVertexBufferPtr vertexBuffer = nullptr;
	m_Device->CreateVertexBuffer(vertexBuffer);

	KAABBBox bound;

	for (auto& pos : positions)
	{
		glm::vec4 t = transform * glm::vec4(pos.DEBUG_POSITION, 1.0);
		pos.DEBUG_POSITION = glm::vec3(t.x, t.y, t.z);
		bound.Merge(pos.DEBUG_POSITION, bound);
	}

	vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
	vertexData.vertexBuffers.push_back(vertexBuffer);
	vertexData.vertexStart = 0;
	vertexData.vertexCount = (uint32_t)positions.size();
	vertexData.bound = bound;

	vertexBuffer->InitMemory(positions.size(), sizeof(positions[0]), positions.data());
	vertexBuffer->InitDevice(false);

	auto& subMeshes = pMesh->m_SubMeshes;

	KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
	newSubMesh->InitDebug(DEBUG_PRIMITIVE_TRIANGLE, &vertexData, nullptr, frameInFlight);

	subMeshes.push_back(newSubMesh);

	return true;
}

bool KMeshUtilityImpl::CreateCylinder(KMesh* pMesh, const glm::mat4& transform, float height, float radius, size_t frameInFlight)
{
	const auto sgements = 50;

	std::vector<KVertexDefinition::DEBUG_POS_3F> positions;
	positions.reserve(sgements * 6);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * TWO_PI) * radius;
		float bz = sin((float)i / (float)sgements * TWO_PI) * radius;

		float cx = cos((float)(i + 1) / (float)sgements * TWO_PI) * radius;
		float cz = sin((float)(i + 1) / (float)sgements * TWO_PI) * radius;

		positions.push_back({ glm::vec3(bx, 0, bz) });
		positions.push_back({ glm::vec3(cx, 0, cz) });
		positions.push_back({ glm::vec3(bx, height, bz) });

		positions.push_back({ glm::vec3(cx, height, cz) });
		positions.push_back({ glm::vec3(bx, height, bz) });
		positions.push_back({ glm::vec3(cx, 0, cz) });
	}

	pMesh->UnInit();

	KVertexData& vertexData = pMesh->m_VertexData;

	IKVertexBufferPtr vertexBuffer = nullptr;
	m_Device->CreateVertexBuffer(vertexBuffer);

	KAABBBox bound;

	for (auto& pos : positions)
	{
		glm::vec4 t = transform * glm::vec4(pos.DEBUG_POSITION, 1.0);
		pos.DEBUG_POSITION = glm::vec3(t.x, t.y, t.z);
		bound.Merge(pos.DEBUG_POSITION, bound);
	}

	vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
	vertexData.vertexBuffers.push_back(vertexBuffer);
	vertexData.vertexStart = 0;
	vertexData.vertexCount = (uint32_t)positions.size();
	vertexData.bound = bound;

	vertexBuffer->InitMemory(positions.size(), sizeof(positions[0]), positions.data());
	vertexBuffer->InitDevice(false);

	auto& subMeshes = pMesh->m_SubMeshes;

	KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
	newSubMesh->InitDebug(DEBUG_PRIMITIVE_TRIANGLE, &vertexData, nullptr, frameInFlight);

	subMeshes.push_back(newSubMesh);

	return true;
}

bool KMeshUtilityImpl::CreateCircle(KMesh* pMesh, const glm::mat4& transform, float radius, size_t frameInFlight)
{
	const size_t sgements = 50;

	std::vector<KVertexDefinition::DEBUG_POS_3F> positions;
	positions.reserve(sgements * 2);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * TWO_PI) * radius;
		float bz = sin((float)i / (float)sgements * TWO_PI) * radius;

		float cx = cos((float)(i + 1) / (float)sgements * TWO_PI) * radius;
		float cz = sin((float)(i + 1) / (float)sgements * TWO_PI) * radius;

		positions.push_back({ glm::vec3(bx, 0, bz) });
		positions.push_back({ glm::vec3(cx, 0, cz) });
	}

	pMesh->UnInit();

	KVertexData& vertexData = pMesh->m_VertexData;

	IKVertexBufferPtr vertexBuffer = nullptr;
	m_Device->CreateVertexBuffer(vertexBuffer);

	KAABBBox bound;

	for (auto& pos : positions)
	{
		glm::vec4 t = transform * glm::vec4(pos.DEBUG_POSITION, 1.0);
		pos.DEBUG_POSITION = glm::vec3(t.x, t.y, t.z);
		bound.Merge(pos.DEBUG_POSITION, bound);
	}

	vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
	vertexData.vertexBuffers.push_back(vertexBuffer);
	vertexData.vertexStart = 0;
	vertexData.vertexCount = (uint32_t)positions.size();
	vertexData.bound = bound;

	vertexBuffer->InitMemory(positions.size(), sizeof(positions[0]), positions.data());
	vertexBuffer->InitDevice(false);

	auto& subMeshes = pMesh->m_SubMeshes;

	KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
	newSubMesh->InitDebug(DEBUG_PRIMITIVE_LINE, &vertexData, nullptr, frameInFlight);

	subMeshes.push_back(newSubMesh);

	return true;
}

bool KMeshUtilityImpl::CreateSphere(KMesh* pMesh, const glm::mat4& transform, float radius, size_t frameInFlight)
{
	const auto sgements = 50;

	std::vector<KVertexDefinition::DEBUG_POS_3F> positions;
	positions.reserve(sgements * sgements * 6);

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

			x[0] = radius * cos(angle_y) * cos(angle_xz);
			y[0] = radius * sin(angle_y);
			z[0] = radius * cos(angle_y) * sin(angle_xz);

			x[1] = radius * cos(angle_y + step_y) * cos(angle_xz);
			y[1] = radius * sin(angle_y + step_y);
			z[1] = radius * cos(angle_y + step_y) * sin(angle_xz);

			x[2] = radius* cos(angle_y + step_y) * cos(angle_xz + step_xz);
			y[2] = radius* sin(angle_y + step_y);
			z[2] = radius* cos(angle_y + step_y) * sin(angle_xz + step_xz);

			x[3] = radius * cos(angle_y) * cos(angle_xz + step_xz);
			y[3] = radius * sin(angle_y);
			z[3] = radius * cos(angle_y) * sin(angle_xz + step_xz);

			positions.push_back({ glm::vec3(x[0], y[0], z[0]) });
			positions.push_back({ glm::vec3(x[1], y[1], z[1]) });
			positions.push_back({ glm::vec3(x[2], y[2], z[2]) });

			positions.push_back({ glm::vec3(x[0], y[0], z[0]) });
			positions.push_back({ glm::vec3(x[2], y[2], z[2]) });
			positions.push_back({ glm::vec3(x[3], y[3], z[3]) });
		}
	}

	pMesh->UnInit();

	KVertexData& vertexData = pMesh->m_VertexData;

	IKVertexBufferPtr vertexBuffer = nullptr;
	m_Device->CreateVertexBuffer(vertexBuffer);

	KAABBBox bound;

	for (auto& pos : positions)
	{
		glm::vec4 t = transform * glm::vec4(pos.DEBUG_POSITION, 1.0);
		pos.DEBUG_POSITION = glm::vec3(t.x, t.y, t.z);
		bound.Merge(pos.DEBUG_POSITION, bound);
	}

	vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
	vertexData.vertexBuffers.push_back(vertexBuffer);
	vertexData.vertexStart = 0;
	vertexData.vertexCount = (uint32_t)positions.size();
	vertexData.bound = bound;

	vertexBuffer->InitMemory(positions.size(), sizeof(positions[0]), positions.data());
	vertexBuffer->InitDevice(false);

	auto& subMeshes = pMesh->m_SubMeshes;

	KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
	newSubMesh->InitDebug(DEBUG_PRIMITIVE_TRIANGLE, &vertexData, nullptr, frameInFlight);

	subMeshes.push_back(newSubMesh);

	return true;
}

bool KMeshUtilityImpl::CreateArc(KMesh* pMesh, const glm::vec3& axis, const glm::vec3& normal,
	float radius, float theta, size_t frameInFlight)
{
	const size_t sgements = 50;

	std::vector<KVertexDefinition::DEBUG_POS_3F> positions;
	positions.reserve(sgements * 3);

	glm::vec3 xAxis = glm::normalize(axis);
	glm::vec3 zAxis = glm::normalize(normal);
	glm::vec3 yAxis = glm::cross(zAxis, xAxis);

	for (auto i = 0; i < sgements; ++i)
	{
		float bx = cos((float)i / (float)sgements * theta) * radius;
		float by = sin((float)i / (float)sgements * theta) * radius;

		float cx = cos((float)(i + 1) / (float)sgements * theta) * radius;
		float cy = sin((float)(i + 1) / (float)sgements * theta) * radius;

		positions.push_back({ bx * xAxis + by * yAxis });
		positions.push_back({ cx * xAxis + cy * yAxis });
		positions.push_back({ glm::vec3(0, 0, 0) });
	}

	pMesh->UnInit();

	KVertexData& vertexData = pMesh->m_VertexData;

	IKVertexBufferPtr vertexBuffer = nullptr;
	m_Device->CreateVertexBuffer(vertexBuffer);

	KAABBBox bound;

	for (auto& pos : positions)
	{
		//glm::vec4 t = transform * glm::vec4(pos.DEBUG_POSITION, 1.0);
		//pos.DEBUG_POSITION = glm::vec3(t.x, t.y, t.z);
		bound.Merge(pos.DEBUG_POSITION, bound);
	}

	vertexData.vertexFormats.push_back(VF_DEBUG_POINT);
	vertexData.vertexBuffers.push_back(vertexBuffer);
	vertexData.vertexStart = 0;
	vertexData.vertexCount = (uint32_t)positions.size();
	vertexData.bound = bound;

	vertexBuffer->InitMemory(positions.size(), sizeof(positions[0]), positions.data());
	vertexBuffer->InitDevice(false);

	auto& subMeshes = pMesh->m_SubMeshes;

	KSubMeshPtr newSubMesh = KSubMeshPtr(new KSubMesh(pMesh));
	newSubMesh->InitDebug(DEBUG_PRIMITIVE_TRIANGLE, &vertexData, nullptr, frameInFlight);

	subMeshes.push_back(newSubMesh);

	return true;
}