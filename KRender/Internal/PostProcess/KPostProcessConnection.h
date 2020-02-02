#pragma once
#include "KPostProcessData.h"
#include "KBase/Interface/IKJson.h"
#include <unordered_set>

class KPostProcessPass;
class KPostProcessManager;

class KPostProcessConnection
{
	friend class KPostProcessManager;
	friend class KPostProcessPass;
	typedef std::string IDType;
protected:
	KPostProcessManager* m_Mgr;
	KPostProcessOutputData m_Output;
	KPostProcessInputData m_Input;

	IDType m_ID;

	static const char* msIDKey;
	static const char* msInKey;
	static const char* msOutKey;

	KPostProcessConnection(KPostProcessManager* mgr);
	KPostProcessConnection(KPostProcessManager* mgr, IDType id);
	~KPostProcessConnection();

	bool Save(IKJsonDocumentPtr jsonDoc, IKJsonValuePtr& object);
	bool Load(IKJsonValuePtr& object);

	inline IDType ID() { return m_ID; }
public:
	inline void SetOutputAsPass(KPostProcessPass* pass, int16_t slot) { m_Output.InitAsPass(pass, slot); }
	inline void SetOutputAsTextrue(IKTexturePtr texture, int16_t slot) { m_Output.InitAsTexture(texture, slot); }
	inline void SetInput(KPostProcessPass* pass, int16_t slot) { m_Input.Init(pass, slot); }
	inline bool IsComplete() const { return m_Input.IsComplete() && m_Output.IsComplete(); }
};

typedef std::unordered_set<KPostProcessConnection*> ConnectionSet;
