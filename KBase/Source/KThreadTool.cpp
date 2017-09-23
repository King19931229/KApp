#include "../Internal/KThreadTool.h"

#ifdef _WIN32
#include <Windows.h>
#else
#endif

namespace KThreadTool
{
	void SetThreadName(char const* pszName, unsigned long uThreadID)
	{
#ifdef _WIN32
#pragma pack(push,8)  
		typedef struct tagTHREADNAME_INFO  
		{  
			DWORD dwType; // Must be 0x1000.  
			LPCSTR pszName; // Pointer to name (in user addr space).  
			DWORD dwThreadID; // Thread ID (-1=caller thread).  
			DWORD dwFlags; // Reserved for future use, must be zero.  
		} THREADNAME_INFO;  
#pragma pack(pop)

		const DWORD MS_VC_EXCEPTION = 0x406D1388;

		THREADNAME_INFO info;
		info.dwType = 0x1000;
		info.pszName = pszName;
		info.dwThreadID = uThreadID;
		info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
		__try{
			RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
		}
		__except (EXCEPTION_EXECUTE_HANDLER){
		}  
#pragma warning(pop)  
#else

#endif
	}
}