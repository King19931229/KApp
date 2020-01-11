#pragma once
#include "Interface/IKTexture.h"

enum PostProcessInputType
{
	POST_PROCESS_INPUT_PASS,
	POST_PROCESS_INPUT_TEXTURE,
	POST_PROCESS_INPUT_UNKNOWN
};

class KPostProcessPass;
class KPostProcessManager;

class KPostProcessConnection
{
	friend class KPostProcessManager;
	friend class KPostProcessPass;
protected:
	PostProcessInputType	m_InputType;
	KPostProcessPass*		m_InputPass;
	IKTexturePtr			m_InputTexture;
	size_t					m_InputSlot;
	KPostProcessPass*		m_OutputPass;

	KPostProcessConnection();
	~KPostProcessConnection();
public:
	void SetInputAsPass(size_t slot, KPostProcessPass* pass);
	void SetInputAsTextrue(size_t slot, IKTexturePtr texture);
	void SetOutput(KPostProcessPass* pass);

	bool IsComplete();
};
