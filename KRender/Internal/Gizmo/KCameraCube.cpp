#include "KCameraCube.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"

IKCameraCubePtr CreateCameraCube()
{
	return IKCameraCubePtr(new KCameraCube());
}

static constexpr float FACE_SIZE = 1.0f;
static constexpr float EDGE_SIZE = 0.4f;
static constexpr float CORE_SIZE = FACE_SIZE - EDGE_SIZE;

const VertexFormat KCameraCube::ms_VertexFormats[] = { VF_DEBUG_POINT };

const KVertexDefinition::DEBUG_POS_3F KCameraCube::ms_BackGroundVertices[] =
{
	glm::vec3(-1.0f, -1.0f, 0.0f),
	glm::vec3(1.0f, -1.0f, 0.0f),
	glm::vec3(1.0f, 1.0f, 0.0f),
	glm::vec3(-1.0f, 1.0f, 0.0f)
};

const uint16_t KCameraCube::ms_BackGroundIndices[] = { 0, 1, 2, 2, 3, 0 };

const KVertexDefinition::DEBUG_POS_3F KCameraCube::ms_CubeVertices[] =
{
	glm::vec3(-1.0, -1.0f, -1.0f) * FACE_SIZE,
	glm::vec3(-1.0, 1.0f, -1.0f) * FACE_SIZE,
	glm::vec3(1.0f, 1.0f, -1.0f) * FACE_SIZE,
	glm::vec3(1.0f, -1.0f, -1.0f) * FACE_SIZE,

	glm::vec3(1.0f, 1.0f, 1.0f) * FACE_SIZE,
	glm::vec3(-1.0, 1.0f, 1.0f * FACE_SIZE),
	glm::vec3(-1.0, -1.0f, 1.0f) * FACE_SIZE,
	glm::vec3(1.0f, -1.0f, 1.0f) * FACE_SIZE,
};

const uint16_t KCameraCube::ms_CubeIndices[] =
{
	// up
	5, 1, 2, 2, 4, 5,
	// down
	6, 3, 0, 3, 6, 7,
	// left
	0, 1, 5, 0, 5, 6,
	// right
	7, 4, 2, 7, 2, 3,
	// front
	6, 5, 4, 6, 4, 7,
	// back
	0, 2, 1, 2, 0, 3,
};

const KCameraCube::CubeFaceInformation KCameraCube::ms_CubeFaceInformation[] =
{
	// top
	{
		CubePart::TOP_RIGHT_EDGE, CubePart::TOP_LEFT_EDGE,
		CubePart::TOP_FRONT_EDGE, CubePart::TOP_BACK_EDGE,
		CubePart::TOP_RIGHT_FRONT_CORNER, CubePart::TOP_RIGHT_BACK_CORNER,
		CubePart::TOP_LEFT_FRONT_CORNER, CubePart::TOP_LEFT_BACK_CORNER,
		CubePart::TOP_FACE,
	},
	// bottom
	{
		CubePart::BOTTOM_RIGHT_EDGE, CubePart::BOTTOM_LEFT_EDGE,
		CubePart::BOTTOM_FRONT_EDGE, CubePart::BOTTOM_BACK_EDGE,
		CubePart::BOTTOM_RIGHT_FRONT_CORNER, CubePart::BOTTOM_RIGHT_BACK_CORNER,
		CubePart::BOTTOM_LEFT_FRONT_CORNER, CubePart::BOTTOM_LEFT_BACK_CORNER,
		CubePart::BOTTOM_FACE,
	},
	// left
	{
		CubePart::FRONT_LEFT_EDGE, CubePart::BACK_LEFT_EDGE,
		CubePart::TOP_LEFT_EDGE, CubePart::BOTTOM_LEFT_EDGE,
		CubePart::TOP_LEFT_FRONT_CORNER, CubePart::BOTTOM_LEFT_FRONT_CORNER,
		CubePart::TOP_LEFT_BACK_CORNER, CubePart::BOTTOM_LEFT_BACK_CORNER,
		CubePart::LEFT_FACE,
	},
	// right
	{
		CubePart::FRONT_RIGHT_EDGE, CubePart::BACK_RIGHT_EDGE,
		CubePart::TOP_RIGHT_EDGE, CubePart::BOTTOM_RIGHT_EDGE,
		CubePart::TOP_RIGHT_FRONT_CORNER, CubePart::BOTTOM_RIGHT_FRONT_CORNER,
		CubePart::TOP_RIGHT_BACK_CORNER, CubePart::BOTTOM_RIGHT_BACK_CORNER,
		CubePart::RIGHT_FACE,
	},
	// front
	{
		CubePart::FRONT_RIGHT_EDGE, CubePart::FRONT_LEFT_EDGE,
		CubePart::TOP_FRONT_EDGE, CubePart::BOTTOM_FRONT_EDGE,
		CubePart::TOP_RIGHT_FRONT_CORNER, CubePart::BOTTOM_RIGHT_FRONT_CORNER,
		CubePart::TOP_LEFT_FRONT_CORNER, CubePart::BOTTOM_LEFT_FRONT_CORNER,
		CubePart::FRONT_FACE,
	},
	// back
	{
		CubePart::BACK_RIGHT_EDGE, CubePart::BACK_LEFT_EDGE,
		CubePart::TOP_BACK_EDGE, CubePart::BOTTOM_BACK_EDGE,
		CubePart::TOP_RIGHT_BACK_CORNER, CubePart::BOTTOM_RIGHT_BACK_CORNER,
		CubePart::TOP_LEFT_BACK_CORNER, CubePart::BOTTOM_LEFT_BACK_CORNER,
		CubePart::BACK_FACE,
	},
};

const char* KCameraCube::CubeFaceToString(CubeFace face)
{
#define ENUM(f) case CubeFace::##f: return #f;
	switch (face)
	{
		ENUM(TOP);
		ENUM(BOTTOM);
		ENUM(LEFT);
		ENUM(RIGHT);
		ENUM(FRONT);
		ENUM(BACK);
	}
#undef ENUM
	assert(false && "impossible to reach");
	return "UNKNOWN";
}

const char* KCameraCube::CubePartToString(KCameraCube::CubePart part)
{
#define ENUM(p) case CubePart::##p: return #p;
	switch (part)
	{
		// 6 face
		ENUM(TOP_FACE);
		ENUM(BOTTOM_FACE);
		ENUM(LEFT_FACE);
		ENUM(RIGHT_FACE);
		ENUM(FRONT_FACE);
		ENUM(BACK_FACE);
		// 12 edge
		ENUM(TOP_LEFT_EDGE);
		ENUM(TOP_RIGHT_EDGE);
		ENUM(TOP_FRONT_EDGE);
		ENUM(TOP_BACK_EDGE);

		ENUM(BOTTOM_LEFT_EDGE);
		ENUM(BOTTOM_RIGHT_EDGE);
		ENUM(BOTTOM_FRONT_EDGE);
		ENUM(BOTTOM_BACK_EDGE);

		ENUM(FRONT_LEFT_EDGE);
		ENUM(FRONT_RIGHT_EDGE);

		ENUM(BACK_LEFT_EDGE);
		ENUM(BACK_RIGHT_EDGE);
		// 8 corner
		ENUM(TOP_LEFT_FRONT_CORNER);
		ENUM(TOP_LEFT_BACK_CORNER);
		ENUM(TOP_RIGHT_FRONT_CORNER);
		ENUM(TOP_RIGHT_BACK_CORNER);

		ENUM(BOTTOM_LEFT_FRONT_CORNER);
		ENUM(BOTTOM_LEFT_BACK_CORNER);
		ENUM(BOTTOM_RIGHT_FRONT_CORNER);
		ENUM(BOTTOM_RIGHT_BACK_CORNER);

		ENUM(NONE);
	}
#undef ENUM
	assert(false && "impossible to reach");
	return "UNKNOWN";
}

const uint16_t KCameraCube::ms_EdgeIndices[] =
{
	// up
	0, 1, 2, 2, 3, 0,
	// down
	3, 2, 5, 5, 4, 3
};

const uint16_t KCameraCube::ms_CornerIndices[] =
{
	// up
	2, 1, 0, 0, 3, 2,
	// back
	2, 3, 4, 5, 2, 4,
	// right
	5, 6, 1, 1, 2, 5
};

bool KCameraCube::ms_Init = false;

KVertexDefinition::DEBUG_POS_3F KCameraCube::ms_EdgeVertices[12][6];
KVertexDefinition::DEBUG_POS_3F KCameraCube::ms_CornerVertices[8][7];

glm::mat4 KCameraCube::ms_Transform[26];

void KCameraCube::PopulateEdge(const glm::vec3& center, const glm::vec3& xAxis, const glm::vec3& yAxis, const glm::vec3& zAxis, KVertexDefinition::DEBUG_POS_3F edge[6])
{
	edge[0] = { center + xAxis * CORE_SIZE + yAxis * CORE_SIZE };
	edge[1] = { center + xAxis * CORE_SIZE - yAxis * CORE_SIZE };
	edge[2] = { center + xAxis * FACE_SIZE - yAxis * CORE_SIZE };
	edge[3] = { center + xAxis * FACE_SIZE + yAxis * CORE_SIZE };
	edge[4] = { center + xAxis * FACE_SIZE + yAxis * CORE_SIZE - zAxis * EDGE_SIZE };
	edge[5] = { center + xAxis * FACE_SIZE - yAxis * CORE_SIZE - zAxis * EDGE_SIZE };
}

void KCameraCube::PopulateCorner(const glm::vec3& center, const glm::vec3& xAxis, const glm::vec3& yAxis, const glm::vec3& zAxis, KVertexDefinition::DEBUG_POS_3F corner[7])
{
	corner[0] = { center + xAxis * CORE_SIZE + yAxis * CORE_SIZE };
	corner[1] = { center + xAxis * FACE_SIZE + yAxis * CORE_SIZE };
	corner[2] = { center + xAxis * FACE_SIZE + yAxis * FACE_SIZE };
	corner[3] = { center + xAxis * CORE_SIZE + yAxis * FACE_SIZE };
	corner[4] = { center + xAxis * CORE_SIZE + yAxis * FACE_SIZE - zAxis * EDGE_SIZE};
	corner[5] = { center + xAxis * FACE_SIZE + yAxis * FACE_SIZE - zAxis * EDGE_SIZE };
	corner[6] = { center + xAxis * FACE_SIZE + yAxis * CORE_SIZE - zAxis * EDGE_SIZE };
}

constexpr static glm::vec3 X_AXIS = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr static glm::vec3 Y_AXIS = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr static glm::vec3 Z_AXIS = glm::vec3(0.0f, 0.0f, 1.0f);

KCameraCube::KCameraCube()
	: m_ScreenWidth(0),
	m_ScreenHeight(0),
	m_DisplayScale(1.0f),
	m_DisplayWidth(0.0),
	m_DisplayHeight(0.0),
	m_Camera(nullptr),
	m_CurrentPick(CubePart::NONE),
	m_LerpTime(0.0f),
	m_HoverIn(false),
	m_MouseDown(false),
	m_CameraLerping(false)
{
	m_CubeCamera.SetPerspective(glm::radians(45.0f), 1.0f, 1.0f, 40.0f);
	UpdateDisplaySize();

	ZERO_ARRAY_MEMORY(m_EdgeVertexBuffer);
	ZERO_ARRAY_MEMORY(m_CornerVertexBuffer);
	ZERO_ARRAY_MEMORY(m_LastMousePos);
	ZERO_ARRAY_MEMORY(m_LastMouseDownPos);

	if (!ms_Init)
	{
		// Populate Edge Vertex
		PopulateEdge(glm::vec3(0.0f, FACE_SIZE, 0.0f), -X_AXIS, -Z_AXIS, Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::TOP_LEFT]);
		PopulateEdge(glm::vec3(0.0f, FACE_SIZE, 0.0f), X_AXIS, Z_AXIS, Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::TOP_RIGHT]);
		PopulateEdge(glm::vec3(0.0f, FACE_SIZE, 0.0f), Z_AXIS, -X_AXIS, Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::TOP_FRONT]);
		PopulateEdge(glm::vec3(0.0f, FACE_SIZE, 0.0f), -Z_AXIS, X_AXIS, Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::TOP_BACK]);

		PopulateEdge(glm::vec3(0.0f, -FACE_SIZE, 0.0f), -X_AXIS, Z_AXIS, -Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::BOTTOM_LEFT]);
		PopulateEdge(glm::vec3(0.0f, -FACE_SIZE, 0.0f), X_AXIS, -Z_AXIS, -Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::BOTTOM_RIGHT]);
		PopulateEdge(glm::vec3(0.0f, -FACE_SIZE, 0.0f), Z_AXIS, X_AXIS, -Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::BOTTOM_FRONT]);
		PopulateEdge(glm::vec3(0.0f, -FACE_SIZE, 0.0f), -Z_AXIS, -X_AXIS, -Y_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::BOTTOM_BACK]);

		PopulateEdge(glm::vec3(0.0f, 0.0f, FACE_SIZE), -X_AXIS, Y_AXIS, Z_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::FRONT_LEFT]);
		PopulateEdge(glm::vec3(0.0f, 0.0f, FACE_SIZE), X_AXIS, -Y_AXIS, Z_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::FRONT_RIGHT]);

		PopulateEdge(glm::vec3(0.0f, 0.0, -FACE_SIZE), -X_AXIS, -Y_AXIS, -Z_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::BACK_LEFT]);
		PopulateEdge(glm::vec3(0.0f, 0.0f, -FACE_SIZE), X_AXIS, Y_AXIS, -Z_AXIS, ms_EdgeVertices[(uint32_t)CubeEdge::BACK_RIGHT]);

		// Pupulate Corner Vertex
		PopulateCorner(glm::vec3(0.0f, FACE_SIZE, 0.0f), -X_AXIS, Z_AXIS, Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::TOP_LEFT_FRONT]);
		PopulateCorner(glm::vec3(0.0f, FACE_SIZE, 0.0f), -Z_AXIS, -X_AXIS, Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::TOP_LEFT_BACK]);
		PopulateCorner(glm::vec3(0.0f, FACE_SIZE, 0.0f), Z_AXIS, X_AXIS, Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::TOP_RIGHT_FRONT]);
		PopulateCorner(glm::vec3(0.0f, FACE_SIZE, 0.0f), X_AXIS, -Z_AXIS, Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::TOP_RIGHT_BACK]);

		PopulateCorner(glm::vec3(0.0f, -FACE_SIZE, 0.0f), Z_AXIS, -X_AXIS, -Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::BOTTOM_LEFT_FRONT]);
		PopulateCorner(glm::vec3(0.0f, -FACE_SIZE, 0.0f), -X_AXIS, -Z_AXIS, -Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::BOTTOM_LEFT_BACK]);
		PopulateCorner(glm::vec3(0.0f, -FACE_SIZE, 0.0f), X_AXIS, Z_AXIS, -Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::BOTTOM_RIGHT_FRONT]);
		PopulateCorner(glm::vec3(0.0f, -FACE_SIZE, 0.0f), -Z_AXIS, X_AXIS, -Y_AXIS, ms_CornerVertices[(uint32_t)CubeCorner::BOTTOM_RIGHT_BACK]);

#define MAKE_VIEW_TRANSFORM(forward, up, mat)\
{\
	glm::vec3 normForward = glm::normalize(forward);\
	glm::vec3 normRight = glm::normalize(glm::cross(forward, up));\
	glm::vec3 normUp = glm::normalize(glm::cross(normRight,normForward));\
	mat = glm::lookAt(-normForward, glm::vec3(0.0f), normUp);\
}
		// 6 face
		MAKE_VIEW_TRANSFORM(glm::vec3(0.0f, -1.0f, 0.0f) - glm::vec3(0.0f, 0.0f, 0.0001f), glm::vec3(0.0f, 0.0f, -1.0f), ms_Transform[(uint32_t)CubePart::TOP_FACE]);
		MAKE_VIEW_TRANSFORM(glm::vec3(0.0f, 1.0f, 0.0f) - glm::vec3(0.0f, 0.0f, 0.0001f), glm::vec3(0.0f, 0.0f, 1.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_FACE]);

		MAKE_VIEW_TRANSFORM(glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::LEFT_FACE]);
		MAKE_VIEW_TRANSFORM(glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::RIGHT_FACE]);
		MAKE_VIEW_TRANSFORM(glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::FRONT_FACE]);
		MAKE_VIEW_TRANSFORM(glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BACK_FACE]);

		// 12 edge
		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_LEFT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_RIGHT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(0.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_FRONT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(0.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_BACK_EDGE]);

		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_LEFT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_RIGHT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(0.0f, -1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_FRONT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(0.0f, -1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_BACK_EDGE]);

		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::FRONT_LEFT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::FRONT_RIGHT_EDGE]);

		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BACK_LEFT_EDGE]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BACK_RIGHT_EDGE]);

		// 8 corner
		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_LEFT_FRONT_CORNER]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_LEFT_BACK_CORNER]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_RIGHT_FRONT_CORNER]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::TOP_RIGHT_BACK_CORNER]);

		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_LEFT_FRONT_CORNER]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_LEFT_BACK_CORNER]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_RIGHT_FRONT_CORNER]);
		MAKE_VIEW_TRANSFORM(-glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f), ms_Transform[(uint32_t)CubePart::BOTTOM_RIGHT_BACK_CORNER]);

		ms_Init = true;
	}
}

KCameraCube::~KCameraCube()
{
}

void KCameraCube::LoadResource()
{
	ASSERT_RESULT(m_VertexShader->InitFromFile("Shaders/cameracube.vert", false));
	ASSERT_RESULT(m_FragmentShader->InitFromFile("Shaders/cameracube.frag", false));
	
	m_BackGroundVertexBuffer->InitMemory(ARRAY_SIZE(ms_BackGroundVertices), sizeof(ms_BackGroundVertices[0]), ms_BackGroundVertices);
	m_BackGroundVertexBuffer->InitDevice(false);

	m_BackGroundIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_BackGroundIndices), ms_BackGroundIndices);
	m_BackGroundIndexBuffer->InitDevice(false);

	m_CubeVertexBuffer->InitMemory(ARRAY_SIZE(ms_CubeVertices), sizeof(ms_CubeVertices[0]), ms_CubeVertices);
	m_CubeVertexBuffer->InitDevice(false);

	m_CubeIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_CubeIndices), ms_CubeIndices);
	m_CubeIndexBuffer->InitDevice(false);

	for (uint16_t i = 0; i < ARRAY_SIZE(m_EdgeVertexBuffer); ++i)
	{
		m_EdgeVertexBuffer[i]->InitMemory(ARRAY_SIZE(ms_EdgeVertices[i]), sizeof(ms_EdgeVertices[i][0]), ms_EdgeVertices[i]);
		m_EdgeVertexBuffer[i]->InitDevice(false);
	}
	m_EdgeIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_EdgeIndices), ms_EdgeIndices);
	m_EdgeIndexBuffer->InitDevice(false);

	for (uint16_t i = 0; i < ARRAY_SIZE(m_CornerVertexBuffer); ++i)
	{
		m_CornerVertexBuffer[i]->InitMemory(ARRAY_SIZE(ms_CornerVertices[i]), sizeof(ms_CornerVertices[i][0]), ms_CornerVertices[i]);
		m_CornerVertexBuffer[i]->InitDevice(false);
	}
	m_CornerIndexBuffer->InitMemory(IT_16, ARRAY_SIZE(ms_CornerIndices), ms_CornerIndices);
	m_CornerIndexBuffer->InitDevice(false);
}

void KCameraCube::PreparePipeline()
{
	for (size_t i = 0; i < m_BackGroundPipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_BackGroundPipelines[i];
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetBlendEnable(true);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);

		pipeline->SetDepthFunc(CF_ALWAYS, false, false);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(ConstantBlock));

		ASSERT_RESULT(pipeline->Init());
	}

	for (size_t i = 0; i < m_CubePipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_CubePipelines[i];
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetBlendEnable(true);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);

		pipeline->SetDepthFunc(CF_LESS, true, true);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(ConstantBlock));

		ASSERT_RESULT(pipeline->Init());
	}

	for (size_t i = 0; i < m_PickPipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_PickPipelines[i];
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetBlendEnable(true);
		pipeline->SetCullMode(CM_BACK);
		pipeline->SetFrontFace(FF_CLOCKWISE);

		pipeline->SetDepthFunc(CF_ALWAYS, false, false);
		pipeline->SetShader(ST_VERTEX, m_VertexShader);
		pipeline->SetShader(ST_FRAGMENT, m_FragmentShader);

		pipeline->CreateConstantBlock(ST_VERTEX, sizeof(ConstantBlock));

		ASSERT_RESULT(pipeline->Init());
	}
}

void KCameraCube::InitRenderData()
{
	m_BackGroundVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_BackGroundVertexBuffer);
	m_BackGroundVertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	m_BackGroundVertexData.vertexCount = ARRAY_SIZE(ms_BackGroundVertices);
	m_BackGroundVertexData.vertexStart = 0;

	m_BackGroundIndexData.indexBuffer = m_BackGroundIndexBuffer;
	m_BackGroundIndexData.indexCount = ARRAY_SIZE(ms_BackGroundIndices);
	m_BackGroundIndexData.indexStart = 0;

	m_CubeVertexData.vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_CubeVertexBuffer);
	m_CubeVertexData.vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
	m_CubeVertexData.vertexCount = ARRAY_SIZE(ms_CubeVertices);
	m_CubeVertexData.vertexStart = 0;

	for (uint32_t i = 0; i < ARRAY_SIZE(m_CubeIndexData); ++i)
	{
		m_CubeIndexData[i].indexBuffer = m_CubeIndexBuffer;
		m_CubeIndexData[i].indexCount = 6;
		m_CubeIndexData[i].indexStart = i * 6;
		m_FaceIndexData[i] = m_CubeIndexData[i];
	}

	for (uint32_t i = 0; i < ARRAY_SIZE(m_EdgeVertexData); ++i)
	{
		m_EdgeVertexData[i].vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_EdgeVertexBuffer[i]);
		m_EdgeVertexData[i].vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
		m_EdgeVertexData[i].vertexCount = ARRAY_SIZE(ms_EdgeVertices[i]);
		m_EdgeVertexData[i].vertexStart = 0;
	}

	m_EdgeIndexData.indexBuffer = m_EdgeIndexBuffer;
	m_EdgeIndexData.indexCount = ARRAY_SIZE(ms_EdgeIndices);
	m_EdgeIndexData.indexStart = 0;

	for (uint32_t i = 0; i < ARRAY_SIZE(m_CornerVertexData); ++i)
	{
		m_CornerVertexData[i].vertexBuffers = std::vector<IKVertexBufferPtr>(1, m_CornerVertexBuffer[i]);
		m_CornerVertexData[i].vertexFormats = std::vector<VertexFormat>(ms_VertexFormats, ms_VertexFormats + ARRAY_SIZE(ms_VertexFormats));
		m_CornerVertexData[i].vertexCount = ARRAY_SIZE(ms_CornerVertices[i]);
		m_CornerVertexData[i].vertexStart = 0;
	}

	m_CornerIndexData.indexBuffer = m_CornerIndexBuffer;
	m_CornerIndexData.indexCount = ARRAY_SIZE(ms_CornerIndices);
	m_CornerIndexData.indexStart = 0;
}

bool KCameraCube::Init(IKRenderDevice* renderDevice, size_t frameInFlight, KCamera* camera)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(frameInFlight > 0);

	ASSERT_RESULT(UnInit());

	m_Camera = camera;

	renderDevice->CreateCommandPool(m_CommandPool);
	m_CommandPool->Init(QUEUE_FAMILY_INDEX_GRAPHICS);

	renderDevice->CreateShader(m_VertexShader);
	renderDevice->CreateShader(m_FragmentShader);

	renderDevice->CreateVertexBuffer(m_BackGroundVertexBuffer);
	renderDevice->CreateIndexBuffer(m_BackGroundIndexBuffer);

	renderDevice->CreateVertexBuffer(m_CubeVertexBuffer);
	renderDevice->CreateIndexBuffer(m_CubeIndexBuffer);

	for (uint16_t i = 0; i < ARRAY_SIZE(m_EdgeVertexBuffer); ++i)
	{
		renderDevice->CreateVertexBuffer(m_EdgeVertexBuffer[i]);
	}
	renderDevice->CreateIndexBuffer(m_EdgeIndexBuffer);

	for (uint16_t i = 0; i < ARRAY_SIZE(m_CornerVertexBuffer); ++i)
	{
		renderDevice->CreateVertexBuffer(m_CornerVertexBuffer[i]);
	}
	renderDevice->CreateIndexBuffer(m_CornerIndexBuffer);

	m_BackGroundPipelines.resize(frameInFlight);
	m_CubePipelines.resize(frameInFlight);
	m_PickPipelines.resize(frameInFlight);
	m_CommandBuffers.resize(frameInFlight);
	m_ClearCommandBuffers.resize(frameInFlight);

	for (size_t i = 0; i < frameInFlight; ++i)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(m_BackGroundPipelines[i]);
		KRenderGlobal::PipelineManager.CreatePipeline(m_CubePipelines[i]);
		KRenderGlobal::PipelineManager.CreatePipeline(m_PickPipelines[i]);

		{
			IKCommandBufferPtr& buffer = m_CommandBuffers[i];
			ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
			ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
		}

		{
			IKCommandBufferPtr& buffer = m_ClearCommandBuffers[i];
			ASSERT_RESULT(renderDevice->CreateCommandBuffer(buffer));
			ASSERT_RESULT(buffer->Init(m_CommandPool, CBL_SECONDARY));
		}
	}

	LoadResource();
	PreparePipeline();
	InitRenderData();

	return true;
}

bool KCameraCube::UnInit()
{
	m_Camera = nullptr;

	for (IKPipelinePtr pipeline : m_BackGroundPipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_BackGroundPipelines.clear();

	for (IKPipelinePtr pipeline : m_CubePipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_CubePipelines.clear();

	for (IKPipelinePtr pipeline : m_PickPipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_PickPipelines.clear();

	for (IKCommandBufferPtr& buffer : m_CommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_CommandBuffers.clear();

	for (IKCommandBufferPtr& buffer : m_ClearCommandBuffers)
	{
		SAFE_UNINIT(buffer);
	}
	m_ClearCommandBuffers.clear();

	SAFE_UNINIT(m_BackGroundVertexBuffer);
	SAFE_UNINIT(m_BackGroundIndexBuffer);
	SAFE_UNINIT(m_CubeVertexBuffer);
	SAFE_UNINIT(m_CubeIndexBuffer);

	SAFE_UNINIT_ARRAY(m_EdgeVertexBuffer);
	SAFE_UNINIT(m_EdgeIndexBuffer);

	SAFE_UNINIT_ARRAY(m_CornerVertexBuffer);
	SAFE_UNINIT(m_CornerIndexBuffer);

	SAFE_UNINIT(m_VertexShader);
	SAFE_UNINIT(m_FragmentShader);

	SAFE_UNINIT(m_CommandPool);

	return true;
}

float KCameraCube::GetDisplayScale() const
{
	return m_DisplayScale;
}

bool KCameraCube::CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir)
{
	float normX = 2.0f * ((float)x / (float)m_ScreenWidth) - 1.0f;
	float normY = 2.0f * ((float)y / (float)m_ScreenHeight) - 1.0f;

	glm::vec4 clipPos = m_InvClipMat * glm::vec4(normX, normY, 0.0f, 1.0f);

	if(fabs(clipPos.z) < 0.0001f && fabs(clipPos.x) <= 1.0f && fabs(clipPos.y) <= 1.0f)
	{
		return m_CubeCamera.CalcPickRay(clipPos.x, clipPos.y, origin, dir);
	}

	return false;
}

bool KCameraCube::PickCubeFace(const glm::vec3& origin, const glm::vec3& dir, CubeFace& face, glm::vec2& projPos)
{
	bool pick = false;;
	float minDistance = 0.0f;

	KPlane plane;
	glm::vec3 intersectPos;

#define DECIDE_FACE(x, y, f)\
{\
	float distance = glm::length(intersectPos - origin);\
	if (fabs(x) <= FACE_SIZE && fabs(y) <= FACE_SIZE)\
	{\
		if (!pick || distance < minDistance)\
		{\
			pick = true;\
			minDistance = distance;\
			face = f;\
			projPos = glm::vec2(x, y);\
		}\
	}\
}

	// top
	plane.Init(glm::vec3(0.0f, FACE_SIZE, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	if (plane.Intersect(origin, dir, intersectPos))
	{
		DECIDE_FACE(intersectPos.x, intersectPos.z, CubeFace::TOP);
	}
	// bottom
	plane.Init(glm::vec3(0.0f, -FACE_SIZE, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
	if (plane.Intersect(origin, dir, intersectPos))
	{
		DECIDE_FACE(intersectPos.x, intersectPos.z, CubeFace::BOTTOM);
	}

	// left
	plane.Init(glm::vec3(-FACE_SIZE, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f));
	if (plane.Intersect(origin, dir, intersectPos))
	{
		DECIDE_FACE(intersectPos.z, intersectPos.y, CubeFace::LEFT);
	}
	// right
	plane.Init(glm::vec3(FACE_SIZE, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	if (plane.Intersect(origin, dir, intersectPos))
	{
		DECIDE_FACE(intersectPos.z, intersectPos.y, CubeFace::RIGHT);
	}

	// front
	plane.Init(glm::vec3(0.0f, 0.0f, FACE_SIZE), glm::vec3(0.0f, 0.0f, FACE_SIZE));
	if (plane.Intersect(origin, dir, intersectPos))
	{
		DECIDE_FACE(intersectPos.x, intersectPos.y, CubeFace::FRONT);
	}
	// back
	plane.Init(glm::vec3(0.0f, 0.0f, -FACE_SIZE), glm::vec3(0.0f, 0.0f, -FACE_SIZE));
	if (plane.Intersect(origin, dir, intersectPos))
	{
		DECIDE_FACE(intersectPos.x, intersectPos.y, CubeFace::BACK);
	}

#undef DECIDE_FACE

	return pick;
}

bool KCameraCube::PickCubePart(CubeFace face, const glm::vec2& projPos, CubePart& part)
{
	if (fabs(projPos[0]) > FACE_SIZE || fabs(projPos[1]) > FACE_SIZE)
	{
		part = CubePart::NONE;
		return false;
	}

	const CubeFaceInformation& information = ms_CubeFaceInformation[(uint32_t)face];

	// right edge
	if (FACE_SIZE - projPos[0] <= EDGE_SIZE)
	{
		// right top corner
		if (FACE_SIZE - projPos[1] <= EDGE_SIZE)
		{
			part = information.corner[0][0];
			return true;
		}
		// right bottom corner
		else if (projPos[1] + FACE_SIZE <= EDGE_SIZE)
		{
			part = information.corner[0][1];
			return true;
		}
		part = information.edge[0];
		return true;
	}
	// left edge
	if (projPos[0] + FACE_SIZE  <= EDGE_SIZE)
	{
		// left top corner
		if (FACE_SIZE - projPos[1] <= EDGE_SIZE)
		{
			part = information.corner[1][0];
			return true;
		}
		// left bottom corner
		else if (projPos[1] + FACE_SIZE <= EDGE_SIZE)
		{
			part = information.corner[1][1];
			return true;
		}
		part = information.edge[1];
		return true;
	}

	// top edge
	if (FACE_SIZE - projPos[1] <= EDGE_SIZE)
	{
		part = information.edge[2];
		return true;
	}
	
	// bottom edge
	if (projPos[1] + FACE_SIZE  <= EDGE_SIZE)
	{
		part = information.edge[3];
		return true;
	}

	// now has to be the face
	part = information.face;

	return true;
}

bool KCameraCube::FindPickRenderData(CubePart part, KVertexData** ppVertexData, KIndexData** ppIndexData)
{
	if (ppVertexData && ppIndexData)
	{
		switch (part)
		{
		case KCameraCube::CubePart::TOP_FACE:
		case KCameraCube::CubePart::BOTTOM_FACE:
		case KCameraCube::CubePart::LEFT_FACE:
		case KCameraCube::CubePart::RIGHT_FACE:
		case KCameraCube::CubePart::FRONT_FACE:
		case KCameraCube::CubePart::BACK_FACE:
		{
			*ppVertexData = &m_CubeVertexData;
			*ppIndexData = &m_FaceIndexData[(uint32_t)part - (uint32_t)CubePart::TOP_FACE];
			return true;
		}
		case KCameraCube::CubePart::TOP_LEFT_EDGE:			
		case KCameraCube::CubePart::TOP_RIGHT_EDGE:
		case KCameraCube::CubePart::TOP_FRONT_EDGE:
		case KCameraCube::CubePart::TOP_BACK_EDGE:
		case KCameraCube::CubePart::BOTTOM_LEFT_EDGE:
		case KCameraCube::CubePart::BOTTOM_RIGHT_EDGE:
		case KCameraCube::CubePart::BOTTOM_FRONT_EDGE:
		case KCameraCube::CubePart::BOTTOM_BACK_EDGE:
		case KCameraCube::CubePart::FRONT_LEFT_EDGE:
		case KCameraCube::CubePart::FRONT_RIGHT_EDGE:
		case KCameraCube::CubePart::BACK_LEFT_EDGE:
		case KCameraCube::CubePart::BACK_RIGHT_EDGE:
		{
			*ppVertexData = &m_EdgeVertexData[(uint32_t)part - (uint32_t)CubePart::TOP_LEFT_EDGE];
			*ppIndexData = &m_EdgeIndexData;
			return true;
		}
		case KCameraCube::CubePart::TOP_LEFT_FRONT_CORNER:			
		case KCameraCube::CubePart::TOP_LEFT_BACK_CORNER:			
		case KCameraCube::CubePart::TOP_RIGHT_FRONT_CORNER:
		case KCameraCube::CubePart::TOP_RIGHT_BACK_CORNER:
		case KCameraCube::CubePart::BOTTOM_LEFT_FRONT_CORNER:
		case KCameraCube::CubePart::BOTTOM_LEFT_BACK_CORNER:
		case KCameraCube::CubePart::BOTTOM_RIGHT_FRONT_CORNER:
		case KCameraCube::CubePart::BOTTOM_RIGHT_BACK_CORNER:
		{
			*ppVertexData = &m_CornerVertexData[(uint32_t)part - (uint32_t)CubePart::TOP_LEFT_FRONT_CORNER];
			*ppIndexData = &m_CornerIndexData;
			return true;
		}
		case KCameraCube::CubePart::NONE:
		default:
			break;
		}
	}
	return false;
}

void KCameraCube::UpdateDisplaySize()
{
	m_DisplayWidth = std::min(std::max(0.001f, m_DisplayScale * 0.45f), 1.0f);
	m_DisplayHeight = m_DisplayWidth;

	float aspect = m_ScreenHeight ? (float)m_ScreenWidth / m_ScreenHeight : 1;
	if (aspect >= 1.0f)
	{
		m_DisplayWidth = m_DisplayHeight / aspect;
	}
	else
	{
		m_DisplayHeight = m_DisplayWidth * aspect;
	}

	m_ClipMat = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 0.0f));
	m_ClipMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f * m_DisplayWidth, 0.5f * m_DisplayHeight, 1.0f)) * m_ClipMat;
	m_ClipMat = glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f + (2.0f - m_DisplayWidth), -1.0, 0.0f)) * m_ClipMat;

	m_InvClipMat = glm::inverse(m_ClipMat);
}

void KCameraCube::SetDisplayScale(float scale)
{
	m_DisplayScale = scale;
	UpdateDisplaySize();
}

void KCameraCube::SetScreenSize(unsigned int width, unsigned int height)
{
	m_ScreenWidth = width;
	m_ScreenHeight = height;
	UpdateDisplaySize();
}

void KCameraCube::OnMouseDown(unsigned int x, unsigned int y)
{
	if (m_CameraLerping)
		return;

	m_LastMouseDownPos[0] = x;
	m_LastMouseDownPos[1] = y;
	m_MouseDown = true;
}

void KCameraCube::GetPickCubePart(unsigned int x, unsigned int y, bool& hoverIn, CubePart& part)
{
	glm::vec3 origin;
	glm::vec3 dir;

	if (CalcPickRay(x, y, origin, dir))
	{
		hoverIn = true;
		CubeFace face;
		glm::vec2 projPos;
		if (PickCubeFace(origin, dir, face, projPos))
		{
			//KG_LOG(LM_DEFAULT, ">Pick face %s", CubeFaceToString(face));
			if (PickCubePart(face, projPos, part))
			{
				//KG_LOG(LM_DEFAULT, ">	Pick part %s", CubePartToString(part));
				return;
			}
		}
		part = CubePart::NONE;
		return;
	}

	hoverIn = false;
	part = CubePart::NONE;
}

void KCameraCube::OnMouseMove(unsigned int x, unsigned int y)
{
	if (m_CameraLerping)
		return;

	GetPickCubePart(x, y, m_HoverIn, m_CurrentPick);

	if (!m_HoverIn)
	{
		m_MouseDown = false;
	}

	if (m_MouseDown)
	{
		int deltaX = x - m_LastMousePos[0];
		int deltaY = y - m_LastMousePos[1];

		float width = m_DisplayWidth * m_ScreenWidth;
		float height = m_DisplayHeight * m_ScreenHeight;

		const float scale = 3.0f;
		if (abs(deltaX) > 0)
		{
			m_Camera->Rotate(glm::vec3(0.0f, 1.0f, 0.0f), scale * -glm::quarter_pi<float>() * deltaX / width);
		}
		if (abs(deltaY) > 0)
		{
			m_Camera->RotateRight(scale * -glm::quarter_pi<float>() * deltaY / height);
		}
	}

	m_LastMousePos[0] = x;
	m_LastMousePos[1] = y;
}

void KCameraCube::OnMouseUp(unsigned int x, unsigned int y)
{
	if (m_CameraLerping)
		return;

	if (m_Camera && m_MouseDown)
	{
		if (x == m_LastMouseDownPos[0] && y == m_LastMouseDownPos[1])
		{
			if (m_CurrentPick != CubePart::NONE)
			{
				m_CameraLerping = true;
				m_LerpTime = 0.0f;
			}
		}
	}
	m_MouseDown = false;
}

static constexpr float CAMERA_LERP_TIME = 0.2f;

#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

void KCameraCube::Update(float dt)
{
	if (m_CameraLerping)
	{
		float timeRemain = CAMERA_LERP_TIME - m_LerpTime;
		assert(timeRemain >= 0.0f);

		glm::vec3 lastPos = m_Camera->GetPostion();
		m_Camera->SetPosition(glm::vec3(0.0f));

		const glm::mat4& viewMat = ms_Transform[(int32_t)m_CurrentPick];

		glm::quat beg = glm::quat_cast(glm::transpose(m_Camera->GetViewMatrix()));
		glm::quat end = glm::quat_cast(glm::transpose(viewMat));
		float lerpRatio = glm::clamp(dt / timeRemain, 0.0f, 1.0f);

		glm::quat final = glm::slerp(beg, end, lerpRatio);

		m_Camera->SetViewMatrix(glm::transpose(glm::mat4_cast(final)));
		m_Camera->SetPosition(lastPos);

		m_LerpTime += dt;
		if (m_LerpTime >= CAMERA_LERP_TIME)
		{
			GetPickCubePart(m_LastMousePos[0], m_LastMousePos[1], m_HoverIn, m_CurrentPick);
			m_CameraLerping = false;
		}
	}
}

const glm::vec3 KCameraCube::CubeFaceColor[] =
{
	glm::vec3(1.0f, 1.0f, 0.0f),
	glm::vec3(1.0f, 1.0f, 1.0f),
	glm::vec3(0.54f, 0.0f, 0.0f),
	glm::vec3(1.0f, 0.38, 0.0f),
	glm::vec3(0.0f, 0.54f, 0.0f),
	glm::vec3(0.0f, 0.0f, 0.54f)
};

bool KCameraCube::GetRenderCommand(size_t frameIndex, KRenderCommandList& commands)
{
	if (frameIndex < m_BackGroundPipelines.size())
	{
		KRenderCommand command;
		ConstantBlock constant;

		m_CubeCamera.SetViewMatrix(m_Camera->GetViewMatrix());
		m_CubeCamera.SetPosition(-m_CubeCamera.GetForward() * 5.0f);

		// BackGround
		if (m_HoverIn)
		{
			command.vertexData = &m_BackGroundVertexData;
			command.indexData = &m_BackGroundIndexData;
			command.pipeline = m_BackGroundPipelines[frameIndex];
			constant.viewprojclip = m_ClipMat;
			constant.color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
			command.SetObjectData(constant);
			command.indexDraw = true;
			commands.push_back(std::move(command));
		}

		// Cube
		{
			command.vertexData = &m_CubeVertexData;
			command.pipeline = m_CubePipelines[frameIndex];
			command.indexDraw = true;
			constant.viewprojclip = m_ClipMat * m_CubeCamera.GetProjectiveMatrix() * m_CubeCamera.GetViewMatrix();

			for (uint32_t i = 0; i < ARRAY_SIZE(m_CubeIndexData); ++i)
			{
				command.indexData = &m_CubeIndexData[i];
				constant.color = glm::vec4(CubeFaceColor[i], m_HoverIn ? 0.8f : 0.2f);
				command.SetObjectData(constant);
				commands.push_back(command);
			}
		}

		// Pick
		if (m_CurrentPick != CubePart::NONE)
		{
			KVertexData* vertexData = nullptr;
			KIndexData* indexData = nullptr;
			if (FindPickRenderData(m_CurrentPick, &vertexData, &indexData))
			{
				command.vertexData = vertexData;
				command.indexData = indexData;
				command.pipeline = m_PickPipelines[frameIndex];
				constant.viewprojclip = m_ClipMat * m_CubeCamera.GetProjectiveMatrix() * m_CubeCamera.GetViewMatrix();
				constant.color = glm::vec4(0.8f, 0.8f, 0.8f, 0.8f);
				command.SetObjectData(constant);
				command.indexDraw = true;
				commands.push_back(std::move(command));
			}
		}

		return true;
	}
	return false;
}

void KCameraCube::ClearDepthStencil(IKCommandBufferPtr buffer, IKRenderTargetPtr target, const KClearDepthStencil& value)
{
	KClearRect rect;

	size_t width = 0;
	size_t height = 0;
	target->GetSize(width, height);

	rect.width = static_cast<uint32_t>(width);
	rect.height = static_cast<uint32_t>(height);

	buffer->ClearDepthStencil(rect, value);
}

bool KCameraCube::Render(size_t frameIndex, IKRenderTargetPtr target, std::vector<IKCommandBufferPtr>& buffers)
{
	KRenderCommandList commands;
	if (GetRenderCommand(frameIndex, commands))
	{
		KClearValue clearValue = { { 0,0,0,0 },{ 1, 0 } };

		IKCommandBufferPtr clearCommandBuffer = m_ClearCommandBuffers[frameIndex];

		clearCommandBuffer->BeginSecondary(target);
		clearCommandBuffer->SetViewport(target);
		ClearDepthStencil(clearCommandBuffer, target, clearValue.depthStencil);
		clearCommandBuffer->End();
		buffers.push_back(clearCommandBuffer);

		IKCommandBufferPtr commandBuffer = m_CommandBuffers[frameIndex];
		commandBuffer->BeginSecondary(target);
		commandBuffer->SetViewport(target);
		for (KRenderCommand& command : commands)
		{
			command.pipeline->GetHandle(target, command.pipelineHandle);			
			commandBuffer->Render(command);			
		}
		commandBuffer->End();
		buffers.push_back(commandBuffer);
		return true;
	}
	return false;
}