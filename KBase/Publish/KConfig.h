#pragma once

#include <stdio.h>
#include <string.h>

#ifdef DLL_EXPORT
#	ifdef DLL_EXPORT
#		define EXPORT_DLL _declspec(dllexport)
#	else
#		define EXPORT_DLL _declspec(dllimport)
#	endif
#else
#	define EXPORT_DLL
#endif

#define EXTERN_C extern "C"
#define STDCALL __stdcall
#define CDECL __cdecl

#ifdef _MSC_VER
#	define ALIGNMENT(x) __declspec(align(x))
#else
#	define ALIGNMENT(x) __attribute__((aligned(x)))
#endif

#define MEMBER_OFFSET(structure, member) ((int)&((structure*)0)->member)
#define MEMBER_SIZE(structure, member) (sizeof(((structure*)0)->member))

#define POINTER_OFFSET(p, offset) ((void*)((char*)p + offset))

#define ZERO_MEMORY(variable) { memset(&variable, 0, sizeof(variable)); }

#define ASSERT_RESULT(exp)\
do\
{\
	bool result = (bool)(exp);\
	assert(result);\
}\
while(false);

// http://www.cnblogs.com/skynet/archive/2011/02/20/1959162.html
#ifdef MEMORY_DUMP_DEBUG

#	ifdef _WIN32
#		define _CRTDBG_MAP_ALLOC
#		include <stdlib.h>
#		include <crtdbg.h>

//	��ʼ����ڴ�й©
//	DUMP_MEMORY_LEAK_BEGIN��ʼ�����ڴ����
#	define DUMP_MEMORY_LEAK_BEGIN() {_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);}
//	��������ڴ�й©
//	DUMP_MEMORY_LEAK_END������ڴ�������
#	define DUMP_MEMORY_LEAK_END() {_CrtDumpMemoryLeaks(); _CrtSetDbgFlag(0);}
#	define DUMP_MEMORY_STATUS() {_CrtMemState s; _CrtMemCheckpoint(&s); _CrtMemDumpStatistics(&s);}

#	else
//	https://en.wikipedia.org/wiki/Mtrace
#	include <stdlib.h>
#	include <mcheck.h>
#	define DUMP_MEMORY_LEAK_BEGIN() {mtrace();}
#	define DUMP_MEMORY_LEAK_END() {muntrace();}
#	define DUMP_MEMORY_STATUS()

#	endif

#else

#	define DUMP_MEMORY_LEAK_BEGIN()
#	define DUMP_MEMORY_LEAK_END()
#	define DUMP_MEMORY_STATUS()

#endif//MEMORY_DUMP_DEBUG