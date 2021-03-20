#pragma once

#include <stdio.h>
#include <string>

// TODO 追踪 new delete
#define KNEW new
#define KDELETE delete

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

#if defined(_DEBUG)
#	define FORCEINLINE								inline
#else
#	define FORCEINLINE								__forceinline
#endif

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define MEMBER_OFFSET(structure, member) ((size_t)&((structure*)0)->member)
#define MEMBER_SIZE(structure, member) (sizeof(((structure*)0)->member))

#define POINTER_OFFSET(p, offset) ((void*)((char*)(p) + offset))

#define ZERO_MEMORY(variable) { memset(&variable, 0, sizeof(variable)); }
#define ZERO_ARRAY_MEMORY(arr) { memset(arr, 0, sizeof(arr)); }

#define SAFE_DELETE(ptr) { if(ptr) { KDELETE ptr; ptr = nullptr; } }
#define SAFE_DELETE_ARRAY(ptr) { if(ptr) { KDELETE[] ptr; ptr = nullptr; } }

#define SAFE_UNINIT(ptr) { if(ptr) { ptr->UnInit(); ptr = nullptr; } }
#define SAFE_UNINIT_CONTAINER(container)\
{\
	for(auto& ptr : container)\
	{\
		if(ptr)\
		{\
			ptr->UnInit();\
			ptr = nullptr;\
		}\
	}\
}

#define ASSERT_RESULT(exp)\
do\
{\
	if(!(exp))\
	{\
		assert(false && "assert failure please check");\
	}\
}\
while(false);

#define ACTION_ON_FAILURE(exp, ...)\
do\
{\
	if(!(exp))\
	{\
		__VA_ARGS__;\
	}\
}\
while(false);

#define ACTION_ON_SUCCESS(exp, ...)\
do\
{\
	if((exp))\
	{\
		__VA_ARGS__;\
	}\
}\
while(false);

// http://www.cnblogs.com/skynet/archive/2011/02/20/1959162.html
#ifdef MEMORY_DUMP_DEBUG

#	ifdef _WIN32
#		define _CRTDBG_MAP_ALLOC
#		include <stdlib.h>
#		include <crtdbg.h>

//	开始检测内存泄漏
//	DUMP_MEMORY_LEAK_BEGIN开始跟踪内存分配
#	define DUMP_MEMORY_LEAK_BEGIN() {_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);}
//	结束检测内存泄漏
//	DUMP_MEMORY_LEAK_END是清空内存分配跟踪
#	define DUMP_MEMORY_LEAK_END() {_CrtDumpMemoryLeaks(); _CrtSetDbgFlag(0);}
#	define DUMP_MEMORY_STATUS() {_CrtMemState s; _CrtMemCheckpoint(&s); _CrtMemDumpStatistics(&s);}
#	define DUMP_MEMORY_BREAK(block) { _CrtSetBreakAlloc(block); }

#	else // _WIN32 
// https://en.wikipedia.org/wiki/Mtrace
// https://onestraw.github.io/cprogram/os-macro/
// https://stackoverflow.com/questions/26569064/fatal-error-mcheck-h-file-not-found
//#		ifdef __linux__
//#			include <stdlib.h>
//#			include <mcheck.h>
//#			define DUMP_MEMORY_LEAK_BEGIN() {mtrace();}
//#			define DUMP_MEMORY_LEAK_END() {muntrace();}
//#			define DUMP_MEMORY_STATUS()
//#			define DUMP_MEMORY_BREAK(block)
//#		else
#			define DUMP_MEMORY_LEAK_BEGIN()
#			define DUMP_MEMORY_LEAK_END()
#			define DUMP_MEMORY_STATUS()
#			define DUMP_MEMORY_BREAK(block)
//#		endif
#	endif

#else

#	define DUMP_MEMORY_LEAK_BEGIN()
#	define DUMP_MEMORY_LEAK_END()
#	define DUMP_MEMORY_STATUS()
#	define DUMP_MEMORY_BREAK(block)
#endif//MEMORY_DUMP_DEBUG