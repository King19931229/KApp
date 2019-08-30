#define MEMORY_DUMP_DEBUG
#include "KBase/Publish/KLockFreeQueue.h"
#include "KBase/Publish/KLockQueue.h"
#include "KBase/Publish/KThreadPool.h"
#include "KBase/Publish/KTimer.h"
#include "KBase/Publish/KSemaphore.h"
#include "KBase/Publish/KTaskExecutor.h"
#include "KBase/Publish/KObjectPool.h"

#include "Interface/IKLog.h"
#include "Publish/KHashString.h"
#include "Publish/KDump.h"

#include "Interface/IKCodec.h"
#include "Interface/IKMemory.h"
IKLogPtr pLog;

#include "Interface/IKScript.h"

#include "Python.h"
int main(int argc, char **argv)
{
	IKScriptCorePtr pCore = GetScriptCore(ST_PYTHON27);
	if(pCore->Init())
	{
		std::string content = "import sys\n"
		"print sys.path";
		pCore->RunScriptFromString(content.c_str());
	}
}