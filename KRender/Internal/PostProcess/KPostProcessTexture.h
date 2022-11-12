#pragma once
#include "Interface/IKPostProcess.h"
#include "KBase/Interface/IKJson.h"

class KPostProcessTexture : public IKPostProcessTexture
{
	friend class KPostProcessManager;
protected:
	IDType m_ID;
	std::string m_Path;
	KTextureRef m_Texture;
	std::unordered_set<IKPostProcessConnection*> m_OutputConnection[PostProcessPort::MAX_OUTPUT_SLOT_COUNT];

	static const char* ms_TextureKey;

	KPostProcessTexture();
	KPostProcessTexture(IDType id);
public:
	virtual ~KPostProcessTexture();

	IKPostProcessPass* CastPass() override { return nullptr; }
	IKPostProcessTexture* CastTexture() override { return this; }

	bool AddInputConnection(IKPostProcessConnection* conn, int16_t slot) override;
	bool AddOutputConnection(IKPostProcessConnection* conn, int16_t slot) override;
	bool RemoveInputConnection(IKPostProcessConnection* conn, int16_t slot) override;
	bool RemoveOutputConnection(IKPostProcessConnection* conn, int16_t slot) override;

	bool GetOutputConnection(std::unordered_set<IKPostProcessConnection*>& set, int16_t slot) override;
	bool GetInputConnection(IKPostProcessConnection*& conn, int16_t slot) override;

	bool SetPath(const char* textureFile) override;
	std::string GetPath() override;

	IDType ID() override;

	inline IKTexturePtr GetTexture() { return *m_Texture; }

	bool Save(IKJsonDocumentPtr jsonDoc, IKJsonValuePtr& object);
	bool Load(IKJsonValuePtr& object);

	bool Init() override;
	bool UnInit() override;
};