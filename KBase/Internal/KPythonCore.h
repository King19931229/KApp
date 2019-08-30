#include "Interface/IKScript.h"

class KPythonCore : public IKScriptCore
{
public:
	virtual ~KPythonCore() {}

	virtual bool Init();
	virtual bool UnInit();
	virtual bool RunScriptFromPath(const char* pPath);
	virtual bool RunScriptFromString(const char* pContent);
};