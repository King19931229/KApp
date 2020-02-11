#pragma once
#include "KPostProcessData.h"
#include "KBase/Interface/IKJson.h"
#include "Interface/IKPostProcess.h"
#include <unordered_set>

class KPostProcessPass;
class KPostProcessManager;

class KPostProcessConnection : public IKPostProcessConnection
{
	friend class KPostProcessManager;
	friend class KPostProcessPass;
	typedef std::string IDType;
protected:
	KPostProcessManager* m_Mgr;
	KPostProcessData m_Output;
	KPostProcessData m_Input;

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
	void SetOutputPort(IKPostProcessNode* node, int16_t slot) override { m_Output.Init(node, slot); }
	void SetInputPort(IKPostProcessNode* node, int16_t slot) override { m_Input.Init(node, slot); }
	IKPostProcessNode* GetInputPortNode() override { return m_Input.node; }
	IKPostProcessNode* GetOutputPortNode() override { return m_Output.node; }
	int16_t GetInputSlot() override { return m_Input.slot; }
	int16_t GetOutputSlot() override { return m_Output.slot; }
	bool IsComplete() const override  { return m_Input.IsComplete() && m_Output.IsComplete(); }
};
