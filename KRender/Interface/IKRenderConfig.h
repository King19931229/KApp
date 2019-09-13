#pragma once
#include "KBase/Interface/IKConfig.h"

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

enum VertexFormat
{
	VF_POINT_NORMAL_UV,
	VF_UV2,
	VF_DIFFUSE_SPECULAR,
	VF_TANGENT_BINORMAL,
	VF_BLEND_WEIGHTS_INDICES,
};

enum IndexType
{
	IT_16,
	IT_32
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