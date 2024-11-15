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
	/*
	python27反初始化会导致内存泄漏
	https://docs.python.org/2/c-api/init.html
	Dynamically loaded extension modules loaded by Python are not unloaded.
	Small amounts of memory allocated by the Python interpreter may not be freed (if you find a leak, please report it).
	Memory tied up in circular references between objects is not freed.
	Some memory allocated by extension modules may not be freed. 
	*/
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
			if(pData->GetSize() > 0)
			{
				size_t uSize = pData->GetSize();
				std::shared_ptr<char> pContent(KNEW char[uSize + 1], [](char* p)->void{ KDELETE[] p; });
				if(pData->Read(pContent.get(), uSize))
				{
					pContent.get()[uSize] = 0;
					return RunScriptFromString(pContent.get());
				}
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