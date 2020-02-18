#include "KPostProcessTexture.h"
#include "Internal/KRenderGlobal.h"
#include "KBase/Publish/KStringParser.h"

KPostProcessTexture::KPostProcessTexture()
	: m_Texture(nullptr)
{
	// TODO
	char szTempBuffer[256] = { 0 };
	size_t address = (size_t)this;
	ASSERT_RESULT(KStringParser::ParseFromSIZE_T(szTempBuffer, ARRAY_SIZE(szTempBuffer), &address, 1));
	m_ID = szTempBuffer;
}

KPostProcessTexture::KPostProcessTexture(IDType id)
	: m_ID(id),
	m_Texture(nullptr)
{
}

KPostProcessTexture::~KPostProcessTexture()
{
	for (auto& connSet : m_OutputConnection)
	{
		assert(connSet.empty());
	}
	assert(!m_Texture);
}

bool KPostProcessTexture::AddInputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	assert(false && "should not reach");
	return false;
}

bool KPostProcessTexture::AddOutputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	if (slot < PostProcessPort::MAX_OUTPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		m_OutputConnection[slot].insert(conn);
		return true;
	}
	return false;
}

bool KPostProcessTexture::RemoveInputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	assert(false && "should not reach");
	return false;
}

bool KPostProcessTexture::RemoveOutputConnection(IKPostProcessConnection* conn, int16_t slot)
{
	if (slot < PostProcessPort::MAX_OUTPUT_SLOT_COUNT && slot >= 0 && conn)
	{
		auto it = m_OutputConnection[slot].find((KPostProcessConnection*)conn);
		if (it != m_OutputConnection[slot].end())
		{
			m_OutputConnection[slot].erase(it);
			return true;
		}
	}
	return false;
}

bool KPostProcessTexture::GetOutputConnection(std::unordered_set<IKPostProcessConnection*>& set, int16_t slot)
{
	if (slot < PostProcessPort::MAX_OUTPUT_SLOT_COUNT && slot >= 0)
	{
		set = m_OutputConnection[slot];
		return true;
	}
	return false;
}

bool KPostProcessTexture::GetInputConnection(IKPostProcessConnection*& conn, int16_t slot)
{
	conn = nullptr;
	return false;
}

bool KPostProcessTexture::SetPath(const char* textureFile)
{
	m_Path = textureFile;
	return true;
}

std::string KPostProcessTexture::GetPath()
{
	return m_Path;
}

KPostProcessTexture::IDType KPostProcessTexture::ID()
{
	return m_ID;
}

const char* KPostProcessTexture::ms_TextureKey = "texture";

bool KPostProcessTexture::Save(IKJsonDocumentPtr jsonDoc, IKJsonValuePtr& object)
{
	object = jsonDoc->CreateObject();
	object->AddMember(ms_TextureKey, jsonDoc->CreateString(m_Path.c_str()));
	return true;
}

bool KPostProcessTexture::Load(IKJsonValuePtr& object)
{
	if (object->IsObject())
	{
		m_Path = object->GetMember(ms_TextureKey)->GetString();
		return true;
	}
	else
	{
		return false;
	}
}

bool KPostProcessTexture::Init()
{
	if (!m_Path.empty())
	{
		if (m_Texture)
		{
			KRenderGlobal::TextrueManager.Release(m_Texture);
			m_Texture = nullptr;
		}

		KRenderGlobal::TextrueManager.Acquire(m_Path.c_str(), m_Texture, false);
		return m_Texture != nullptr;
	}
	return false;
}

bool KPostProcessTexture::UnInit()
{
	if (m_Texture)
	{
		KRenderGlobal::TextrueManager.Release(m_Texture);
		m_Texture = nullptr;
	}

	for (auto& connSet : m_OutputConnection)
	{
		connSet.clear();
	}

	return true;
}