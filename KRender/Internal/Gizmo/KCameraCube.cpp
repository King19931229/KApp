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

KCameraCube::KCameraCube()
	: m_ScreenWidth(0),
	m_ScreenHeight(0),
	m_DisplayScale(1.0f),
	m_Camera(nullptr)
{
}

KCameraCube::~KCameraCube()
{
}

void KCameraCube::PreparePipeline()
{
}

void KCameraCube::InitRenderData()
{
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

	if (m_BackGroundVertexBuffer)
	{
		m_BackGroundVertexBuffer->UnInit();
		m_BackGroundVertexBuffer = nullptr;
	}
	if (m_BackGroundIndexBuffer)
	{
		m_BackGroundIndexBuffer->UnInit();
		m_BackGroundIndexBuffer = nullptr;
	}

	if (m_CubeVertexBuffer)
	{
		m_CubeVertexBuffer->UnInit();
		m_CubeVertexBuffer = nullptr;
	}
	if (m_CubeIndexBuffer)
	{
		m_CubeIndexBuffer->UnInit();
		m_CubeIndexBuffer = nullptr;
	}

	if (m_VertexShader)
	{
		m_VertexShader->UnInit();
		m_VertexShader = nullptr;
	}
	if (m_FragmentShader)
	{
		m_FragmentShader->UnInit();
		m_FragmentShader = nullptr;
	}

	return true;
}

float KCameraCube::GetDisplayScale() const
{
	return m_DisplayScale;
}

void KCameraCube::SetDisplayScale(float scale)
{
	m_DisplayScale = scale;
}

void KCameraCube::SetScreenSize(unsigned int width, unsigned int height)
{
	m_ScreenWidth = width;
	m_ScreenHeight = height;
}

void KCameraCube::OnMouseDown(unsigned int x, unsigned int y)
{

}

void KCameraCube::OnMouseMove(unsigned int x, unsigned int y)
{

}

void KCameraCube::OnMouseUp(unsigned int x, unsigned int y)
{

}

bool KCameraCube::GetRenderCommand(unsigned int imageIndex, KRenderCommandList& commands)
{
	return true;
}