#pragma once
#include "KRender/Interface/IKTexture.h"
#include <unordered_set>

const static int16_t MAX_INPUT_SLOT_COUNT = 4;
const static int16_t MAX_OUTPUT_SLOT_COUNT = 4;
static const int16_t INVALID_SLOT_INDEX = -1;

struct IKPostProcessConnection;
struct IKPostProcessPass;
struct IKPostProcessManager;
typedef std::unordered_set<IKPostProcessConnection*> KPostProcessConnectionSet;

struct IKPostProcessConnection
{
	virtual void SetOutputPortAsPass(IKPostProcessPass* pass, int16_t slot) = 0;
	virtual void SetOutputPortAsTextrue(IKTexturePtr texture, int16_t slot) = 0;
	virtual void SetInputPort(IKPostProcessPass* pass, int16_t slot) = 0;
	virtual IKPostProcessPass* GetInputPortPass() = 0;
	virtual IKPostProcessPass* GetOutputPortPass() = 0;
	virtual int16_t GetInputSlot() = 0;
	virtual int16_t GetOutputSlot() = 0;
	virtual bool IsComplete() const = 0;
};

struct IKPostProcessPass
{
	virtual bool SetShader(const char* vsFile, const char* fsFile) = 0;
	virtual bool SetScale(float scale) = 0;
	virtual bool SetFormat(ElementFormat format) = 0;
	virtual bool SetMSAA(unsigned short msaaCount) = 0;

	virtual bool AddInputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool AddOutputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool RemoveInputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool RemoveOutputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;

	virtual bool GetOutputConnection(KPostProcessConnectionSet& set, int16_t slot) = 0;
	virtual bool GetInputConnection(IKPostProcessConnection*& conn, int16_t slot) = 0;
};

struct IKPostProcessManager
{
	virtual IKPostProcessPass* GetStartPointPass() = 0;

	virtual IKPostProcessPass* CreatePass(const char* vsFile, const char* fsFile, float scale, ElementFormat format) = 0;
	virtual void DeletePass(IKPostProcessPass* pass) = 0;

	virtual IKPostProcessConnection* CreatePassConnection(IKPostProcessPass* outputPass, int16_t outSlot, IKPostProcessPass* inputPass, int16_t inSlot) = 0;
	virtual IKPostProcessConnection* CreateTextureConnection(IKTexturePtr outputTexure, int16_t outSlot, IKPostProcessPass* inputPass, int16_t inSlot) = 0;
	virtual void DeleteConnection(IKPostProcessConnection* conn) = 0;
};

EXPORT_DLL IKPostProcessManager* GetProcessManager();