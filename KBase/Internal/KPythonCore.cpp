#include "Internal/KPythonCore.h"
#include "Interface/IKDataStream.h"

#ifdef _WIN32
#include "Python.h"
#endif

bool KPythonCore::Init()
{
#ifdef _WIN32
	Py_Initialize();
	return Py_IsInitialized() == 1;
#else
	return false;
#endif
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
#ifdef _WIN32
	Py_Finalize();
	return Py_IsInitialized() == 0;
#else
	return false;
#endif
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
				std::shared_ptr<char> pContent(new char[uSize + 1], [](char* p)->void{ delete[] p; });
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
#ifdef _WIN32
		int nRet = PyRun_SimpleString(pContent);
		return nRet != 0;
#endif
	}
	return false;
}