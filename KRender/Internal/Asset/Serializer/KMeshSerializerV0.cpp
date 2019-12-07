#include "KMeshSerializerV0.h"

enum MeshSerializerElement
{
	MSE_HEAD = 0x1234,
		MSE_VERSION = 0x01,
		// version number [ushort]
		MSE_VERTEX_DATA = 0x02,
			// vertex start [uint]
			// vertex count [uint]
			MSE_VERTEX_ELEMENT = 0x03,
				// vertex element count [ushort]
				MSE_VERTEX_FORMAT = 0x04,
				// vertex format [ushort]
				MSE_VERTEX_BUFFER = 0x05,
				// vertex buffer count [ushort]
				// vertex buffer size [uint]
				// vertex buffer data [bytes]
		MSE_INDEX_DATA = 0x06,
			// index element count [ushort]
			MSE_INDEX_ELEMENT = 0x07,
				// index count [ushort]
				// is32format [bool]
				// index buffer size [uint]
				// index buffer data [bytes]
};

KMeshSerializerV0::KMeshSerializerV0()
{
}

KMeshSerializerV0::~KMeshSerializerV0()
{
}

bool KMeshSerializerV0::LoadFromFile(KMesh* pMesh, const char* path)
{

}

bool KMeshSerializerV0::SaveAsFile(KMesh* pMesh, const char* path)
{

}
