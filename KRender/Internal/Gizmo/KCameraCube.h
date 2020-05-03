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

	std::vector<IKPipelinePtr> m_BackGroundPipelines;
	std::vector<IKPipelinePtr> m_CubePipelines;

	IKVertexBufferPtr m_BackGroundVertexBuffer;
	IKIndexBufferPtr m_BackGroundIndexBuffer;
	
	IKVertexBufferPtr m_CubeVertexBuffer;
	IKIndexBufferPtr m_CubeIndexBuffer;

	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

	KVertexData m_BackGroundVertexData;
	KIndexData m_BackGroundIndexData;

	KVertexData m_CubeVertexData;
	// top bottom left right front back
	KIndexData m_CubeIndexData[6];

	static const glm::vec3 CubeFaceColor[6];

	enum class CubeFace : uint32_t
	{
		TOP,
		BOTTOM,
		LEFT,
		RIGHT,
		FRONT,
		BACK
	};
	static const char* CubeFaceToString(CubeFace face);

	enum class CubePart : uint32_t
	{
		// 6 face
		TOP_FACE,
		BOTTOM_FACE,
		LEFT_FACE,
		RIGHT_FACE,
		FRONT_FACE,
		BACK_FACE,
		// 12 edge
		TOP_LEFT_EDGE,
		TOP_RIGHT_EDGE,
		TOP_FRONT_EDGE,
		TOP_BACK_EDGE,

		BOTTOM_LEFT_EDGE,
		BOTTOM_RIGHT_EDGE,
		BOTTOM_FRONT_EDGE,
		BOTTOM_BACK_EDGE,

		FRONT_LEFT_EDGE,
		FRONT_RIGHT_EDGE,

		BACK_LEFT_EDGE,
		BACK_RIGHT_EDGE,
		// 8 corner
		TOP_LEFT_FRONT_CORNER,
		TOP_LEFT_BACK_CORNER,
		TOP_RIGHT_FRONT_CORNER,
		TOP_RIGHT_BACK_CORNER,

		BOTTOM_LEFT_FRONT_CORNER,
		BOTTOM_LEFT_BACK_CORNER,
		BOTTOM_RIGHT_FRONT_CORNER,
		BOTTOM_RIGHT_BACK_CORNER,
	};
	static const char* CubePartToString(CubePart part);

	struct CubeFaceInformation
	{
		// [row0]->[x_positive]
		// [row1]->[x_negative]
		// [row2]->[y_positive]
		// [row3]->[y_negative]
		CubePart edge[4];

		// [row0] [0]->[x_positive] [1]->[x_negative]
		// [row1] [0]->[y_positive] [1]->[y_negative]
		CubePart corner[2][2];

		CubePart face;
	};
	static const CubeFaceInformation ms_CubeFaceInformation[6];

	bool CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir);
	bool PickCubeFace(const glm::vec3& origin, const glm::vec3& dir, CubeFace& face, glm::vec2& projPos);
	bool PickCubePart(CubeFace face, const glm::vec2& projPos, CubePart& part);

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