#pragma once
#include "KRender/Interface/IKTexture.h"
#include <unordered_set>

const static int16_t MAX_INPUT_SLOT_COUNT = 4;
const static int16_t MAX_OUTPUT_SLOT_COUNT = 4;
static const int16_t INVALID_SLOT_INDEX = -1;

struct IKPostProcessConnection;
struct IKPostProcessNode;
struct IKPostProcessManager;
typedef std::unordered_set<IKPostProcessConnection*> KPostProcessConnectionSet;
typedef std::unordered_set<IKPostProcessNode*> KPostProcessNodeSet;

struct IKPostProcessConnection
{
	virtual void SetOutputPort(IKPostProcessNode* node, int16_t slot) = 0;
	virtual void SetInputPort(IKPostProcessNode* node, int16_t slot) = 0;
	virtual IKPostProcessNode* GetInputPortNode() = 0;
	virtual IKPostProcessNode* GetOutputPortNode() = 0;
	virtual int16_t GetInputSlot() = 0;
	virtual int16_t GetOutputSlot() = 0;
	virtual bool IsComplete() const = 0;
};

enum PostProcessNodeType
{
	PPNT_PASS,
	PPNT_TEXTURE
};

struct IKPostProcessNode
{
	virtual ~IKPostProcessNode() {}
	virtual PostProcessNodeType GetType() = 0;

	typedef std::string IDType;
	virtual IDType ID() = 0;

	//
	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
	virtual bool AddInputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool AddOutputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool RemoveInputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool RemoveOutputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;

	virtual bool GetOutputConnection(KPostProcessConnectionSet& set, int16_t slot) = 0;
	virtual bool GetInputConnection(IKPostProcessConnection*& conn, int16_t slot) = 0;
};

struct IKPostProcessTexture : public IKPostProcessNode
{
	PostProcessNodeType GetType() override { return PPNT_TEXTURE; }

	virtual bool SetPath(const char* textureFile) = 0;
	virtual std::string GetPath() = 0;
};

struct IKPostProcessPass : public IKPostProcessNode
{
	PostProcessNodeType GetType() override { return PPNT_PASS; }

	virtual bool SetShader(const char* vsFile, const char* fsFile) = 0;
	virtual bool SetScale(float scale) = 0;
	virtual bool SetFormat(ElementFormat format) = 0;
	virtual bool SetMSAA(unsigned short msaaCount) = 0;

	virtual std::tuple<std::string, std::string> GetShader() = 0; // first vs. second fs.
	virtual float GetScale() = 0;
	virtual ElementFormat GetFormat() = 0;
	virtual unsigned short GetMSAA() = 0;
};

struct IKPostProcessManager
{
	virtual IKPostProcessPass* GetStartPointPass() = 0;

	virtual IKPostProcessPass* CreatePass() = 0;
	virtual IKPostProcessTexture* CreateTextrue() = 0;

	virtual void DeleteNode(IKPostProcessNode* pass) = 0;
	virtual IKPostProcessNode* GetNode(IKPostProcessNode::IDType id) = 0;
	virtual bool GetAllNodes(KPostProcessNodeSet& set) = 0;

	virtual IKPostProcessConnection* CreateConnection(IKPostProcessNode* outputNode, int16_t outSlot, IKPostProcessNode* inputNode, int16_t inSlot) = 0;
	virtual void DeleteConnection(IKPostProcessConnection* conn) = 0;
};

EXPORT_DLL IKPostProcessManager* GetProcessManager();