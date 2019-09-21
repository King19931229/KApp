#pragma once
#include "KBase/Publish/KConfig.h"

#include <memory>

enum RenderDevice
{
	RD_VULKAN,
	RD_COUNT
};

enum ShaderType
{
	ST_VERTEX,
	ST_FRAGMENT,
	ST_COUNT
};

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
	VS_BLEND_INDICES
};

enum ConstantBufferType
{
	CBT_TRANSFORM
};

enum ConstantSemantic
{
	CS_MODEL,
	CS_VIEW,
	CS_PROJ
};

enum VertexFormat
{
	VF_POINT_NORMAL_UV,
	VF_UV2,
	VF_DIFFUSE_SPECULAR,
	VF_TANGENT_BINORMAL,
	VF_BLEND_WEIGHTS_INDICES,
};

enum ElementFormat
{
	EF_R8GB8BA8_UNORM,
	EF_R8G8B8A8_SNORM,

	EF_R16_FLOAT,
	EF_R16G16_FLOAT,
	EF_R16G16B16_FLOAT,
	EF_R16G16B16A16_FLOAT,

	EF_R32_FLOAT,
	EF_R32G32_FLOAT,
	EF_R32G32B32_FLOAT,
	EF_R32G32B32A32_FLOAT,

	EF_R32_UINT
};

enum IndexType
{
	IT_16,
	IT_32
};

enum TextureType
{
	TT_TEXTURE_2D
};

struct IKRenderWindow;
typedef std::shared_ptr<IKRenderWindow> IKRenderWindowPtr;

struct IKRenderDevice;
typedef std::shared_ptr<IKRenderDevice> IKRenderDevicePtr;

struct IKShader;
typedef std::shared_ptr<IKShader> IKShaderPtr;

struct IKProgram;
typedef std::shared_ptr<IKProgram> IKProgramPtr;

struct IKVertexBuffer;
typedef std::shared_ptr<IKVertexBuffer> IKVertexBufferPtr;

struct IKIndexBuffer;
typedef std::shared_ptr<IKIndexBuffer> IKIndexBufferPtr;

struct IKUniformBuffer;
typedef std::shared_ptr<IKUniformBuffer> IKUniformBufferPtr;

struct IKTexture;
typedef std::shared_ptr<IKTexture> IKTexturePtr;