#pragma once
#include "KRender/Interface/IKTexture.h"
#include <unordered_set>
#include <memory>

struct PostProcessPort
{
	constexpr static int16_t MAX_INPUT_SLOT_COUNT = 4;
	constexpr static int16_t MAX_OUTPUT_SLOT_COUNT = 4;
	constexpr static int16_t INVALID_SLOT_INDEX = -1;
};

struct IKPostProcessConnection;
struct IKPostProcessNode;
struct IKPostProcessPass;
struct IKPostProcessTexture;
struct IKPostProcessManager;

typedef std::shared_ptr<IKPostProcessConnection> IKPostProcessConnectionPtr;
typedef std::shared_ptr<IKPostProcessNode> IKPostProcessNodePtr;

typedef std::unordered_set<IKPostProcessNodePtr> KPostProcessNodeSet;
typedef std::unordered_set<IKPostProcessConnectionPtr> KPostProcessConnectionSet;

struct IKPostProcessConnection
{
	typedef std::string IDType;

	virtual void SetOutputPort(IKPostProcessNode* node, int16_t slot) = 0;
	virtual void SetInputPort(IKPostProcessNode* node, int16_t slot) = 0;
	virtual IKPostProcessNode* GetInputPortNode() = 0;
	virtual IKPostProcessNode* GetOutputPortNode() = 0;
	virtual int16_t GetInputSlot() = 0;
	virtual int16_t GetOutputSlot() = 0;
	virtual bool IsComplete() const = 0;
	virtual IDType ID() = 0;
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

	virtual bool GetOutputConnection(std::unordered_set<IKPostProcessConnection*>& set, int16_t slot) = 0;
	virtual bool GetInputConnection(IKPostProcessConnection*& conn, int16_t slot) = 0;

	virtual IKPostProcessPass* CastPass() = 0;
	virtual IKPostProcessTexture* CastTexture() = 0;

	// 内部使用 编辑器不能调用
	virtual bool Init() = 0;
	virtual bool UnInit() = 0;

	virtual bool AddInputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool AddOutputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool RemoveInputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
	virtual bool RemoveOutputConnection(IKPostProcessConnection* conn, int16_t slot) = 0;
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
	virtual IKPostProcessNodePtr GetStartPointPass() = 0;

	virtual IKPostProcessNodePtr CreatePass() = 0;
	virtual IKPostProcessNodePtr CreateTextrue() = 0;

	virtual void DeleteNode(IKPostProcessNodePtr node) = 0;
	virtual IKPostProcessNodePtr GetNode(IKPostProcessNode::IDType id) = 0;
	virtual bool GetAllNodes(KPostProcessNodeSet& set) = 0;

	virtual IKPostProcessConnectionPtr FindConnection(IKPostProcessNodePtr outputNode, int16_t outSlot, IKPostProcessNodePtr inputNode, int16_t inSlot) = 0;
	virtual IKPostProcessConnectionPtr CreateConnection(IKPostProcessNodePtr outputNode, int16_t outSlot, IKPostProcessNodePtr inputNode, int16_t inSlot) = 0;
	virtual void DeleteConnection(IKPostProcessConnectionPtr conn) = 0;
	virtual bool GetAllConnections(KPostProcessConnectionSet& set) = 0;

	virtual bool Construct() = 0;
};

EXPORT_DLL IKPostProcessManager* GetProcessManager();