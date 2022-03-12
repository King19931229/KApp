#pragma once
#include "Interface/IKGizmo.h"
#include "Interface/IKCommandBuffer.h"
#include "Internal/KVertexDefinition.h"

class KCameraCube : public IKCameraCube
{
protected:
	unsigned int m_ScreenWidth;
	unsigned int m_ScreenHeight;

	float m_DisplayScale;

	float m_DisplayWidth;
	float m_DisplayHeight;

	KCamera* m_Camera;
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

	// Pipeline
	IKPipelinePtr m_BackGroundPipeline;
	IKPipelinePtr m_CubePipeline;
	IKPipelinePtr m_PickPipeline;

	IKCommandBufferPtr m_CommandBuffer;
	IKCommandBufferPtr m_ClearCommandBuffer;
	IKCommandPoolPtr m_CommandPool;

	// Buffer
	IKVertexBufferPtr m_BackGroundVertexBuffer;
	IKIndexBufferPtr m_BackGroundIndexBuffer;
	
	IKVertexBufferPtr m_CubeVertexBuffer;
	IKIndexBufferPtr m_CubeIndexBuffer;

	IKVertexBufferPtr m_EdgeVertexBuffer[12];
	IKIndexBufferPtr m_EdgeIndexBuffer;

	IKVertexBufferPtr m_CornerVertexBuffer[8];
	IKIndexBufferPtr m_CornerIndexBuffer;

	// Cube Display
	KVertexData m_CubeVertexData;
	KIndexData m_CubeIndexData[6];

	// Hover Display
	KVertexData m_BackGroundVertexData;
	KIndexData m_BackGroundIndexData;

	KIndexData m_FaceIndexData[6];

	KVertexData m_EdgeVertexData[12];
	KIndexData m_EdgeIndexData;

	KVertexData m_CornerVertexData[8];
	KIndexData m_CornerIndexData;

	// Shader
	IKShaderPtr m_VertexShader;
	IKShaderPtr m_FragmentShader;

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

	enum class CubeEdge : uint32_t
	{
		TOP_LEFT,
		TOP_RIGHT,
		TOP_FRONT,
		TOP_BACK,

		BOTTOM_LEFT,
		BOTTOM_RIGHT,
		BOTTOM_FRONT,
		BOTTOM_BACK,

		FRONT_LEFT,
		FRONT_RIGHT,

		BACK_LEFT,
		BACK_RIGHT,
	};

	enum class CubeCorner : uint32_t
	{
		TOP_LEFT_FRONT,
		TOP_LEFT_BACK,
		TOP_RIGHT_FRONT,
		TOP_RIGHT_BACK,

		BOTTOM_LEFT_FRONT,
		BOTTOM_LEFT_BACK,
		BOTTOM_RIGHT_FRONT,
		BOTTOM_RIGHT_BACK,
	};

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

		NONE
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

	static void PopulateEdge(const glm::vec3& center, const glm::vec3& xAxis, const glm::vec3& yAxis, const glm::vec3& zAxis, KVertexDefinition::DEBUG_POS_3F edge[6]);
	static void PopulateCorner(const glm::vec3& center, const glm::vec3& xAxis, const glm::vec3& yAxis, const glm::vec3& zAxis, KVertexDefinition::DEBUG_POS_3F corner[7]);

	static bool ms_Init;
	static KVertexDefinition::DEBUG_POS_3F ms_EdgeVertices[12][6];
	static KVertexDefinition::DEBUG_POS_3F ms_CornerVertices[8][7];

	static const uint16_t ms_EdgeIndices[12];
	static const uint16_t ms_CornerIndices[18];

	static glm::mat4 ms_Transform[26];

	CubePart m_CurrentPick;
	float m_LerpTime;

	bool m_HoverIn;
	bool m_MouseDown;
	bool m_CameraLerping;

	unsigned int m_LastMouseDownPos[2];
	unsigned int m_LastMousePos[2];

	bool CalcPickRay(unsigned int x, unsigned int y, glm::vec3& origin, glm::vec3& dir);
	bool PickCubeFace(const glm::vec3& origin, const glm::vec3& dir, CubeFace& face, glm::vec2& projPos);
	bool PickCubePart(CubeFace face, const glm::vec2& projPos, CubePart& part);	
	bool FindPickRenderData(CubePart part, KVertexData** ppVertexData, KIndexData** ppIndexData);

	void GetPickCubePart(unsigned int x, unsigned int y, bool& hoverIn, CubePart& part);

	void UpdateDisplaySize();
	void LoadResource();
	void PreparePipeline();
	void InitRenderData();

	bool GetRenderCommand(KRenderCommandList& commands);
public:
	KCameraCube();
	~KCameraCube();

	bool Init(IKRenderDevice* renderDevice, size_t frameInFlight, KCamera* camera) override;
	bool UnInit() override;

	float GetDisplayScale() const override;
	void SetDisplayScale(float scale) override;

	void SetScreenSize(unsigned int width, unsigned int height) override;

	void Update(float dt) override;

	void OnMouseDown(unsigned int x, unsigned int y) override;
	void OnMouseMove(unsigned int x, unsigned int y) override;
	void OnMouseUp(unsigned int x, unsigned int y) override;

	bool Render(IKRenderPassPtr renderPass, std::vector<IKCommandBufferPtr>& buffers);
};