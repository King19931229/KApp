﻿#pragma once

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