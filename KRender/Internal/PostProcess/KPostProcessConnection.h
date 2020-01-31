#pragma once
#include "KPostProcessData.h"
#include <unordered_set>

class KPostProcessPass;
class KPostProcessManager;

class KPostProcessConnection
{
	friend class KPostProcessManager;
	friend class KPostProcessPass;
protected:
	KPostProcessOutputData m_Output;
	KPostProcessInputData m_Input;

	KPostProcessConnection();
	~KPostProcessConnection();
public:
	inline void SetOutputAsPass(KPostProcessPass* pass, int16_t slot) { m_Output.InitAsPass(pass, slot); }
	inline void SetOutputAsTextrue(IKTexturePtr texture, int16_t slot) { m_Output.InitAsTexture(texture, slot); }
	inline void SetInput(KPostProcessPass* pass, int16_t slot) { m_Input.Init(pass, slot); }
	inline bool IsComplete() const { return m_Input.IsComplete() && m_Output.IsComplete(); }
};

typedef std::unordered_set<KPostProcessConnection*> ConnectionSet;
