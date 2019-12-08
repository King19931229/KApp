#pragma once
#include "KBase/Publish/KConfig.h"

#include <vector>
#include <memory>

enum RenderDevice
{
	RD_VULKAN,
	RD_COUNT
};

enum ShaderTypeFlag
{
	ST_VERTEX = 0x01,
	ST_FRAGMENT = 0x02,
};
typedef unsigned short ShaderTypes;

enum VertexSemantic
{
	VS_POSITION,
	VS_NORMAL,

	VS_UV,
	VS_UV2,

	VS_DIFFUSE,
	VS_SPECULAR,

	VS_TANGENT,
	VS_BINORMAL,

	VS_BLEND_WEIGHTS,
	VS_BLEND_INDICES,

	VS_GUI_POS,
	VS_GUI_UV,
	VS_GUI_COLOR,

	VS_SCREENQAUD_POS
};

enum ConstantBufferType
{
	CBT_OBJECT,
	CBT_CAMERA,
	CBT_SHADOW,

	CBT_UNKNOWN,
	CBT_COUNT = CBT_UNKNOWN
};

enum ConstantSemantic
{
	CS_MODEL,
	CS_VIEW,

	CS_PROJ,
	CS_VIEW_INV,

	CS_SHADOW_VIEW,
	CS_SHADOW_PROJ,
	CS_SHADOW_NEAR_FAR
};

enum VertexFormat
{
	// for 3d element
	VF_POINT_NORMAL_UV,
	VF_UV2,
	VF_DIFFUSE_SPECULAR,
	VF_TANGENT_BINORMAL,
	VF_BLEND_WEIGHTS_INDICES,
	// for gui
	VF_GUI_POS_UV_COLOR,
	// for offscreen quad
	VF_SCREENQUAD_POS,

	VF_UNKNOWN,
	VF_COUNT = VF_UNKNOWN
};

enum ElementFormat
{
	EF_R8GB8BA8_UNORM,
	EF_R8G8B8A8_SNORM,

	EF_R8GB8B8_UNORM,

	EF_R16_FLOAT,
	EF_R16G16_FLOAT,
	EF_R16G16B16_FLOAT,
	EF_R16G16B16A16_FLOAT,

	EF_R32_FLOAT,
	EF_R32G32_FLOAT,
	EF_R32G32B32_FLOAT,
	EF_R32G32B32A32_FLOAT,

	EF_R32_UINT,

	EF_ETC1_R8G8B8_UNORM,
	EF_ETC2_R8G8B8_UNORM,
	EF_ETC2_R8G8B8A1_UNORM,
	EF_ETC2_R8G8B8A8_UNORM,

	EF_UNKNOWN,
	EF_COUNT = EF_UNKNOWN
};

enum IndexType
{
	IT_16,
	IT_32
};

enum TextureType
{
	TT_TEXTURE_2D,
	TT_TEXTURE_CUBE_MAP,

	TT_UNKNOWN,
	TT_COUNT = TT_UNKNOWN
};

enum AddressMode
{
	AM_REPEAT,
	AM_CLAMP_TO_BORDER,

	AM_UNKNOWN,
	AM_COUNT = AM_UNKNOWN
};

enum FilterMode
{
	FM_NEAREST,
	FM_LINEAR,

	FM_UNKNOWN,
	FM_COUNT = FM_UNKNOWN
};

enum PrimitiveTopology
{
	PT_TRIANGLE_LIST,
	PT_TRIANGLE_STRIP
};

enum PolygonMode
{
	PM_FILL,
	PM_LINE,
	PM_POINT
};

enum CullMode
{
	CM_NONE,
	CM_FRONT,
	CM_BACK
};

enum FrontFace
{
	FF_COUNTER_CLOCKWISE,
	FF_CLOCKWISE
};

enum CompareFunc
{
	CF_NEVER,
	CF_LESS,
	CF_EQUAL,
	CF_LESS_OR_EQUAL,
	CF_GREATER,
	CF_NOT_EQUAL,
	CF_GREATER_OR_EQUAL,
	CF_ALWAYS,
};

enum BlendFactor
{
	BF_ZEOR,
	BF_ONE,

	BF_SRC_COLOR,
	BF_ONE_MINUS_SRC_COLOR,

	BF_SRC_ALPHA,
	BF_ONE_MINUS_SRC_ALPHA
};

enum BlendOperator
{
	BO_ADD,
	BO_SUBTRACT
};

enum RenderTargetComponent
{
	RTC_COLOR,
	RTC_DEPTH_STENCIL
};

enum PipelineStage
{
	PIPELINE_STAGE_OPAQUE,
	PIPELINE_STAGE_SHADOW_GEN,
	PIPELINE_STAGE_COUNT
};

enum QueueFamilyIndex
{
	QUEUE_FAMILY_INDEX_GRAPHICS,
	QUEUE_FAMILY_INDEX_PRESENT
};

enum CommandBufferLevel
{
	CBL_PRIMARY,
	CBL_SECONDARY
};

enum SubpassContents
{
	SUBPASS_CONTENTS_INLINE,
	SUBPASS_CONTENTS_SECONDARY
};

//TODO
struct ImageView
{
	void* imageViewHandle;
	int imageForamt;
	bool fromSwapChain;
	bool fromDepthStencil;
};

struct IKRenderWindow;
typedef std::shared_ptr<IKRenderWindow> IKRenderWindowPtr;

struct IKRenderDevice;
typedef std::shared_ptr<IKRenderDevice> IKRenderDevicePtr;

struct IKShader;
typedef std::shared_ptr<IKShader> IKShaderPtr;

struct IKVertexBuffer;
typedef std::shared_ptr<IKVertexBuffer> IKVertexBufferPtr;

struct IKIndexBuffer;
typedef std::shared_ptr<IKIndexBuffer> IKIndexBufferPtr;

struct IKUniformBuffer;
typedef std::shared_ptr<IKUniformBuffer> IKUniformBufferPtr;

struct IKTexture;
typedef std::shared_ptr<IKTexture> IKTexturePtr;

struct IKSampler;
typedef std::shared_ptr<IKSampler> IKSamplerPtr;

struct IKRenderTarget;
typedef std::shared_ptr<IKRenderTarget> IKRenderTargetPtr;

struct IKPipeline;
typedef std::shared_ptr<IKPipeline> IKPipelinePtr;

struct IKPipelineHandle;
typedef std::shared_ptr<IKPipelineHandle> IKPipelineHandlePtr;

struct IKCommandPool;
typedef std::shared_ptr<IKCommandPool> IKCommandPoolPtr;

struct IKCommandBuffer;
typedef std::shared_ptr<IKCommandBuffer> IKCommandBufferPtr;

struct IKUIOverlay;
typedef std::shared_ptr<IKUIOverlay> IKUIOverlayPtr;