#include "KCameraCube.h"
#include "Internal/KRenderGlobal.h"

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

const KVertexDefinition::DEBUG_POS_3F KCameraCube::ms_CubeVertices[] =
{
	glm::vec3(-1.0, -1.0f, -1.0f),
	glm::vec3(-1.0, 1.0f, -1.0f),
	glm::vec3(1.0f, 1.0f, -1.0f),
	glm::vec3(1.0f, -1.0f, -1.0f),

	glm::vec3(1.0f, 1.0f, 1.0f),
	glm::vec3(-1.0, 1.0f, 1.0f),
	glm::vec3(-1.0, -1.0f, 1.0f),
	glm::vec3(1.0f, -1.0f, 1.0f),
};

const uint16_t KCameraCube::ms_CubeIndices[] =
{
	// back
	0, 2, 1, 2, 0, 3,
	// front
	6, 5, 4, 6, 4, 7,
	// left
	0, 1, 5, 0, 5, 6,
	// right
	7, 4, 2, 7, 2, 3,
	// up
	5, 1, 2, 2, 4, 5,
	// down
	6, 3, 0, 3, 6, 7
};

const VertexFormat KCameraCube::ms_VertexFormats[] = { VF_DEBUG_POINT };

KCameraCube::KCameraCube()
	: m_ScreenWidth(0),
	m_ScreenHeight(0),
	m_DisplayScale(1.0f),
	m_DisplayWidth(0.0),
	m_DisplayHeight(0.0),
	m_Camera(nullptr)
{
	SetDisplayScale(m_DisplayScale);
	m_CubeCamera.SetPerspective(glm::radians(45.0f), 1.0f, 1.0f, 100.0f);
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
	for (size_t i = 0; i < m_Pipelines.size(); ++i)
	{
		IKPipelinePtr pipeline = m_Pipelines[i];
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

	m_CubeIndexData.indexBuffer = m_CubeIndexBuffer;
	m_CubeIndexData.indexCount = ARRAY_SIZE(ms_CubeIndices);
	m_CubeIndexData.indexStart = 0;
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
	m_Pipelines.resize(numImages);

	for (size_t i = 0; i < numImages; ++i)
	{
		KRenderGlobal::PipelineManager.CreatePipeline(m_Pipelines[i]);
	}

	LoadResource();
	PreparePipeline();
	InitRenderData();

	return true;
}

bool KCameraCube::UnInit()
{
	m_Camera = nullptr;

	for (IKPipelinePtr pipeline : m_Pipelines)
	{
		KRenderGlobal::PipelineManager.DestroyPipeline(pipeline);
		pipeline = nullptr;
	}
	m_Pipelines.clear();

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

	assert(fabs(clipPos.z) < 0.0001f);

	if (fabs(clipPos.x) <= 1.0f && fabs(clipPos.y) <= 1.0f)
	{
		return m_CubeCamera.CalcPickRay(clipPos.x, clipPos.y, origin, dir);
	}
	else
	{
		return false;
	}
}

void KCameraCube::UpdateDisplaySize()
{
	m_DisplayWidth = std::min(std::max(0.0f, m_DisplayScale * 0.6f), 1.0f);
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
		//TODO
	}
}

void KCameraCube::OnMouseUp(unsigned int x, unsigned int y)
{

}

bool KCameraCube::GetRenderCommand(unsigned int imageIndex, KRenderCommandList& commands)
{
	if (imageIndex < m_Pipelines.size())
	{
		KRenderCommand command;
		ConstantBlock constant;
		
		// BackGround
		{
			command.vertexData = &m_BackGroundVertexData;
			command.indexData = &m_BackGroundIndexData;
			command.pipeline = m_Pipelines[imageIndex];
			constant.viewprojclip = m_ClipMat;
			constant.color = glm::vec4(1.0f, 1.0f, 0.0f, 0.5f);
			command.SetObjectData(constant);
			command.indexDraw = true;
			commands.push_back(std::move(command));
		}

		// Cube
		{
			m_CubeCamera.SetViewMatrix(m_Camera->GetViewMatrix());
			m_CubeCamera.SetPosition(-m_CubeCamera.GetForward() * 5.0f);

			command.vertexData = &m_CubeVertexData;
			command.indexData = &m_CubeIndexData;
			command.pipeline = m_Pipelines[imageIndex];
			constant.viewprojclip = m_ClipMat * m_CubeCamera.GetProjectiveMatrix() * m_CubeCamera.GetViewMatrix();
			constant.color = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);
			command.SetObjectData(constant);
			command.indexDraw = true;
			commands.push_back(std::move(command));
		}

		return true;
	}
	return false;
}