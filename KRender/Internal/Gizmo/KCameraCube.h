#pragma once
#include "Interface/IKGizmo.h"
#include "Internal/KVertexDefinition.h"

class KCameraCube : public IKCameraCube
{
protected:
	unsigned int m_ScreenWidth;
	unsigned int m_ScreenHeight;

	float m_DisplayScale;

	float m_DisplayWidth;
	float m_DisplayHeight;

	const KCamera* m_Camera;
	KCamera m_CubeCamera;

	glm::mat4 m_ClipMat;
	glm::mat4 m_InvClipMat;

	struct ConstantBlock
	{
		glm::mat4 viewprojclip;
		glm::vec4 color;
	};

	static const KVertexDefinition::DEBUG_POS_3F ms_BackGroundVertices[4];
	static const uint16_t ms_BackGroundIndices[6];

	static const KVertexDefinition::DEBUG_POS_3F ms_CubeVertices[8];
	static const uint16_t ms_CubeIndices[36];

	static const VertexFormat ms_VertexFormats[1];

	std::vector<IKPipelinePtr> m_Pipelines;

	IKVertexBufferPtr m_BackGroundVertexBuffer;
	IKIndexBufferPtr m_BackGroundIndexBuffer;
	
	IKVertexBufferPtr m_CubeVertexBuffer;
	IKIndexBufferPtr m_CubeIndexBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	KVertexData m_BackGroundVertexData;
	KIndexData m_BackGroundIndexData;

	KVertexData m_CubeVertexData;
	KIndexData m_CubeIndexData;

	bool CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir);

	void UpdateDisplaySize();
	void LoadResource();
	void PreparePipeline();
	void InitRenderData();
public:
	KCameraCube();
	~KCameraCube();

	bool Init(IKRenderDevice* renderDevice, size_t frameInFlight, const KCamera* camera) override;
	bool UnInit() override;

	float GetDisplayScale() const override;
	void SetDisplayScale(float scale) override;

	void SetScreenSize(unsigned int width, unsigned int height) override;

	void OnMouseDown(unsigned int x, unsigned int y) override;
	void OnMouseMove(unsigned int x, unsigned int y) override;
	void OnMouseUp(unsigned int x, unsigned int y) override;

	bool GetRenderCommand(unsigned int imageIndex, KRenderCommandList& commands);
};