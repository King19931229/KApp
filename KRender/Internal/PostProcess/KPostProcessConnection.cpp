#include "KPostProcessConnection.h"
#include "KPostProcessPass.h"
#include "KPostProcessManager.h"

KPostProcessConnection::KPostProcessConnection()
	: m_InputType(POST_PROCESS_INPUT_UNKNOWN),
	m_InputPass(nullptr),
	m_InputTexture(nullptr),
	m_InputSlot(0),
	m_OutputPass(nullptr)
{
}

KPostProcessConnection::~KPostProcessConnection()
{
}

void KPostProcessConnection::SetInputAsPass(size_t slot, KPostProcessPass* pass)
{
	assert(pass);
	m_InputSlot = slot;
	m_InputType = POST_PROCESS_INPUT_PASS;
	m_InputPass = pass;
	m_InputTexture = nullptr;
}

void KPostProcessConnection::SetInputAsTextrue(size_t slot, IKTexturePtr texture)
{
	assert(texture);
	m_InputSlot = slot;
	m_InputType = POST_PROCESS_INPUT_TEXTURE;
	m_InputPass = nullptr;
	m_InputTexture = texture;
}

void KPostProcessConnection::SetOutput(KPostProcessPass* pass)
{
	assert(pass);
	m_OutputPass = pass;
}

bool KPostProcessConnection::IsComplete()
{
	return (m_InputPass != nullptr || m_InputTexture != nullptr) && m_OutputPass != nullptr;
}