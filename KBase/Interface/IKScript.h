#pragma once
#include "Publish/KConfig.h"
#include <memory>
#include <string>

enum ScriptType
{
	ST_PYTHON27
};

struct IKScriptCore
{
public:
	virtual ~IKScriptCore() {}

	virtual bool Init() = 0;
	virtual bool UnInit() = 0;
	virtual bool RunScriptFromPath(const char* pPath) = 0;
	virtual bool RunScriptFromString(const char* pContent) = 0;
};

typedef std::shared_ptr<IKScriptCore> IKScriptCorePtr;
EXPORT_DLL IKScriptCorePtr GetScriptCore(ScriptType type);