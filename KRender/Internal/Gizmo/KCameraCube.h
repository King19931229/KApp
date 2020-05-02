#pragma once
#include "Interface/IKGizmo.h"
#include "Internal/KVertexDefinition.h"

class KCameraCube : public IKCameraCube
{
protected:
	unsigned int m_ScreenWidth;
	unsigned int m_ScreenHeight;	
	float m_DisplayScale;
	const KCamera* m_Camera;

	static const KVertexDefinition::DEBUG_POS_3F ms_BackGroundVertices[4];
	static const uint16_t ms_BackGroundIndices[6];

	static const KVertexDefinition::DEBUG_POS_3F ms_CubeVertices[8];
	static const uint16_t ms_CubeIndices[36];

	std::vector<IKPipelinePtr> m_BackGroundPipelines;
	IKVertexBufferPtr m_BackGroundVertexBuffer;
	IKIndexBufferPtr m_BackGroundIndexBuffer;
	
	std::vector<IKPipelinePtr> m_CubePipelines;
	IKVertexBufferPtr m_CubeVertexBuffer;
	IKIndexBufferPtr m_CubeIndexBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

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