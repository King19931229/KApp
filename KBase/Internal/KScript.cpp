#include "Interface/IKScript.h"
#include "Internal/KPythonCore.h"

IKScriptCorePtr GetScriptCore(ScriptType type)
{
	switch (type)
	{
	case ST_PYTHON27:
		return IKScriptCorePtr(new KPythonCore());
	default:
		return IKScriptCorePtr(nullptr);
	}
}