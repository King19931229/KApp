#include "KCameraCube.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Interface/IKLog.h"

IKCameraCubePtr CreateCameraCube()
{
	return IKCameraCubePtr(new KCameraCube());
}

const KVertexDefinition::DEBUG_POS_3F KCameraCube::ms_BackGroundVertices[] =
{
	glm::vec3(-1.0f, -1.0f, 0.0f),
	glm::vec3(1.0f, -1.0f, 0.0f),
	glm::vec3(1.0f, 1.0f, 0.0f),
	glm::vec3(-1.0f, 1.0f, 0.0f)
};

const uint16_t KCameraCube::ms_BackGroundIndices[] = { 0, 1, 2, 2, 3, 0 };

static constexpr float FACE_SIZE = 1.0f;
static constexpr float EDGE_SIZE = 0.4f;

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

const VertexFormat KCameraCube::ms_VertexFormats[] = { VF_DEBUG_POINT };

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
	}
#undef ENUM
	assert(false && "impossible to reach");
	return "UNKNOWN";
}

KCameraCube::KCameraCube()
	: m_ScreenWidth(0),
	m_ScreenHeight(0),
	m_DisplayScale(1.0f),
	m_DisplayWidth(0.0),
	m_DisplayHeight(0.0),
	m_Camera(nullptr)
{
	m_CubeCamera.SetPerspective(glm::radians(45.0f), 1.0f, 1.0f, 40.0f);
	UpdateDisplaySize();
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
}

void KCameraCube::PreparePipeline()
{
	for (size_t i = 0; i < m_BackGroundPipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_BackGroundPipelines[i];
		pipeline->SetVertexBinding(ms_VertexFormats, ARRAY_SIZE(ms_VertexFormats));
		pipeline->SetPrimitiveTopology(PT_TRIANGLE_LIST);

		pipeline->SetBlendEnable(true);
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetPolygonMode(PM_FILL);

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
		pipeline->SetCullMode(CM_NONE);
		pipeline->SetPolygonMode(PM_FILL);

		pipeline->SetDepthFunc(CF_LESS_OR_EQUAL, true, true);
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
	}
}

bool KCameraCube::Init(IKRenderDevice* renderDevice, size_t frameInFlight, const KCamera* camera)
{
	ASSERT_RESULT(renderDevice != nullptr);
	ASSERT_RESULT(frameInFlight > 0);

	ASSERT_RESULT(UnInit());

	m_Camera = camera;

	renderDevice->CreateShader(m_VertexShader);
	renderDevice->CreateShader(m_FragmentShader);

	renderDevice->CreateVertexBuffer(m_BackGroundVertexBuffer);
	renderDevice->CreateIndexBuffer(m_BackGroundIndexBuffer);

	renderDevice->CreateVertexBuffer(m_CubeVertexBuffer);
	renderDevice->CreateIndexBuffer(m_CubeIndexBuffer);

	size_t numImages = frameInFlight;
	m_BackGroundPipelines.resize(numImages);
	m_CubePipelines.resize(numImages);

	for (size_t i = 0; i < numImages; ++i)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(m_BackGroundPipelines[i]);
		KRenderGlobal::PipelineManager.CreatePipeline(m_CubePipelines[i]);
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

	SAFE_UNINIT(m_BackGroundVertexBuffer);
	SAFE_UNINIT(m_BackGroundIndexBuffer);
	SAFE_UNINIT(m_CubeVertexBuffer);
	SAFE_UNINIT(m_CubeIndexBuffer);
	SAFE_UNINIT(m_VertexShader);
	SAFE_UNINIT(m_FragmentShader);

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

void KCameraCube::UpdateDisplaySize()
{
	m_DisplayWidth = std::min(std::max(0.001f, m_DisplayScale * 0.6f), 1.0f);
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

}

void KCameraCube::OnMouseMove(unsigned int x, unsigned int y)
{
	glm::vec3 origin;
	glm::vec3 dir;

	if (CalcPickRay(x, y, origin, dir))
	{
		CubeFace face;
		glm::vec2 projPos;
		if (PickCubeFace(origin, dir, face, projPos))
		{
			KG_LOG(LM_DEFAULT, ">Pick face %s", CubeFaceToString(face));
			CubePart part;
			if (PickCubePart(face, projPos, part))
			{
				KG_LOG(LM_DEFAULT, ">	Pick part %s", CubePartToString(part));
			}
		}
	}
}

void KCameraCube::OnMouseUp(unsigned int x, unsigned int y)
{

}

const glm::vec3 KCameraCube::CubeFaceColor[] =
{
	glm::vec3(1.0f, 1.0f, 0.0f),
	glm::vec3(1.0f, 1.0f, 1.0f),
	glm::vec3(1.0f, 0.0f, 0.0f),
	glm::vec3(1.0f, 0.38, 0.0f),
	glm::vec3(0.0f, 1.0f, 0.0f),
	glm::vec3(0.0f, 0.0f, 1.0f)
};

bool KCameraCube::GetRenderCommand(unsigned int imageIndex, KRenderCommandList& commands)
{
	if (imageIndex < m_BackGroundPipelines.size())
	{
		KRenderCommand command;
		ConstantBlock constant;
		
		// BackGround
		{
			command.vertexData = &m_BackGroundVertexData;
			command.indexData = &m_BackGroundIndexData;
			command.pipeline = m_BackGroundPipelines[imageIndex];
			constant.viewprojclip = m_ClipMat;
			constant.color = glm::vec4(0.5f, 0.5f, 0.5f, 0.5f);
			command.SetObjectData(constant);
			command.indexDraw = true;
			commands.push_back(std::move(command));
		}

		// Cube
		{
			m_CubeCamera.SetViewMatrix(m_Camera->GetViewMatrix());
			m_CubeCamera.SetPosition(-m_CubeCamera.GetForward() * 5.0f);

			command.vertexData = &m_CubeVertexData;
			command.pipeline = m_CubePipelines[imageIndex];
			command.indexDraw = true;
			constant.viewprojclip = m_ClipMat * m_CubeCamera.GetProjectiveMatrix() * m_CubeCamera.GetViewMatrix();

			for (uint32_t i = 0; i < ARRAY_SIZE(m_CubeIndexData); ++i)
			{
				command.indexData = &m_CubeIndexData[i];
				constant.color = glm::vec4(CubeFaceColor[i], 0.95f);
				command.SetObjectData(constant);
				commands.push_back(command);
			}
		}

		return true;
	}
	return false;
}