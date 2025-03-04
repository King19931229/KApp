// Static Constant Buffer Binding
CONSTANT_BUFFER_BINDING(CAMERA)

CONSTANT_BUFFER_BINDING(SHADOW)

CONSTANT_BUFFER_BINDING(DYNAMIC_CASCADED_SHADOW)
CONSTANT_BUFFER_BINDING(STATIC_CASCADED_SHADOW)

CONSTANT_BUFFER_BINDING(VOXEL)
CONSTANT_BUFFER_BINDING(VOXEL_CLIPMAP)
CONSTANT_BUFFER_BINDING(GLOBAL)

CONSTANT_BUFFER_BINDING(VIRTUAL_TEXTURE_CONSTANT)

// Dynamic Constant Buffer Binding
CONSTANT_BUFFER_BINDING(OBJECT)
CONSTANT_BUFFER_BINDING(SHADING)
CONSTANT_BUFFER_BINDING(VIRTUAL_TEXTURE_FEEDBACK_TARGET)
CONSTANT_BUFFER_BINDING(VIRTUAL_TEXTURE_BINDING)
CONSTANT_BUFFER_BINDING(DEBUG)

// Storage Buffer Binding
//		Vertex Input
#define VERTEX_FORMAT_SCENE(FORMAT) STORAGE_BUFFER_BINDING(FORMAT)
#define VERTEX_FORMAT_OTHER(...)
#include "KVertexFormat.inl"
#undef VERTEX_FORMAT_OTHER
#undef VERTEX_FORMAT_SCENE
//		VirtualTexture Description
STORAGE_BUFFER_BINDING(VIRTUAL_TEXTURE_DESCRIPTION)
//		VirtualTexture Feedback
STORAGE_BUFFER_BINDING(VIRTUAL_TEXTURE_FEEDBACK_RESULT)
//		GPUScene Index Input
STORAGE_BUFFER_BINDING(INDEX)
//		GPUScene Instance Data
STORAGE_BUFFER_BINDING(INSTANCE_DATA)
//		GPUScene Material Parameter
STORAGE_BUFFER_BINDING(MATERIAL_PARAMETER)
//		GPUScene Material Index
STORAGE_BUFFER_BINDING(MATERIAL_INDEX)
//		GPUScene MateraiTexture Binding
STORAGE_BUFFER_BINDING(MATERIAL_TEXTURE_BINDING)
//		GPUScene Mesh State
STORAGE_BUFFER_BINDING(MESH_STATE)
//		GPUScene MegaShader State
STORAGE_BUFFER_BINDING(MEGA_SHADER_STATE)
//		GPUScene Drawing Group
STORAGE_BUFFER_BINDING(DRAWING_GROUP)

// Texture Binding
TEXTURE_BINDING(0)
TEXTURE_BINDING(1)
TEXTURE_BINDING(2)
TEXTURE_BINDING(3)
TEXTURE_BINDING(4)
TEXTURE_BINDING(5)
TEXTURE_BINDING(6)
TEXTURE_BINDING(7)
TEXTURE_BINDING(8)
TEXTURE_BINDING(9)
TEXTURE_BINDING(10)
TEXTURE_BINDING(11)
TEXTURE_BINDING(12)
TEXTURE_BINDING(13)
TEXTURE_BINDING(14)
TEXTURE_BINDING(15)
TEXTURE_BINDING(16)
TEXTURE_BINDING(17)
TEXTURE_BINDING(18)
TEXTURE_BINDING(19)
TEXTURE_BINDING(20)
TEXTURE_BINDING(21)
TEXTURE_BINDING(22)
TEXTURE_BINDING(23)
TEXTURE_BINDING(24)
TEXTURE_BINDING(25)
TEXTURE_BINDING(26)
TEXTURE_BINDING(27)
TEXTURE_BINDING(28)
TEXTURE_BINDING(29)
TEXTURE_BINDING(30)
TEXTURE_BINDING(31)
TEXTURE_BINDING(32)
TEXTURE_BINDING(33)
TEXTURE_BINDING(34)
TEXTURE_BINDING(35)