#include "Internal/KPythonCore.h"
#include "Interface/IKDataStream.h"

#include "Python.h"

bool KPythonCore::Init()
{
	Py_Initialize();
	return Py_IsInitialized() == 1;
}

bool KPythonCore::UnInit()
{
	Py_Finalize();
	return Py_IsInitialized() == 0;
}

bool KPythonCore::RunScriptFromPath(const char* pPath)
{
	if(pPath)
	{
		IKDataStreamPtr pData = GetDataStream(IT_MEMORY);
		if(pData)
		{
			pData->Open(pPath, IM_READ);
			const char* pContent = NULL;
			pData->Reference((char**)&pContent, pData->GetSize());
			if(pContent)
			{
				return RunScriptFromString(pContent);
			}
		}
	}
	return false;
}

bool KPythonCore::RunScriptFromString(const char* pContent)
{
	if(pContent)
	{
		int nRet = PyRun_SimpleString(pContent);
		return nRet != 0;
	}
	return false;
}