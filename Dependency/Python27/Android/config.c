/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void initposix(void);
extern void initfcntl(void);

extern void initarray(void);
extern void initimp(void);
extern void initgc(void);
extern void init_io(void);
extern void init_ast(void);
extern void initbinascii(void);
extern void initcmath(void);
extern void initerrno(void);
extern void initfuture_builtins(void);
extern void initmath(void);
extern void initsignal(void);
extern void initoperator(void);
extern void init_md5(void);
extern void init_sha(void);
extern void init_sha256(void);
extern void init_sha512(void);
extern void initstrop(void);
extern void inittime(void);
extern void initthread(void);
extern void initcStringIO(void);
extern void initcPickle(void);
extern void init_codecs(void);
extern void init_weakref(void);
extern void init_hotshot(void);
extern void init_random(void);
extern void init_bisect(void);
extern void init_lsprof(void);
extern void init_heapq(void);
extern void inititertools(void);
extern void initxxsubtype(void);
extern void initzipimport(void);
extern void init_collections(void);
extern void init_symtable(void);
extern void initmmap(void);
extern void init_csv(void);
extern void init_sre(void);
extern void init_ssl(void);
extern void initparser(void);
extern void init_struct(void);
extern void initdatetime(void);
extern void init_functools(void);
extern void init_json(void);
extern void initzlib(void);
extern void initselect(void);
extern void init_socket(void);
extern void initunicodedata(void);
extern void init_multibytecodec(void);
extern void init_codecs_cn(void);
extern void init_codecs_hk(void);
extern void init_codecs_iso2022(void);
extern void init_codecs_jp(void);
extern void init_codecs_kr(void);
extern void init_codecs_tw(void);

extern void PyMarshal_Init(void);
extern void _PyWarnings_Init(void);

struct _inittab _PyImport_Inittab[] = {

    {"posix", initposix},
    {"fcntl", initfcntl},

    {"array", initarray},
    {"imp", initimp},
    {"gc", initgc},
    {"_io", init_io},
    {"_ast", init_ast},
    {"binascii", initbinascii},
    {"cmath", initcmath},
    {"errno", initerrno},
    {"future_builtins", initfuture_builtins},
    {"math", initmath},
    {"signal", initsignal},
    {"operator", initoperator},
    {"_md5", init_md5},
    {"_sha", init_sha},
    {"_sha256", init_sha256},
    {"_sha512", init_sha512},
    {"strop", initstrop},
    {"time", inittime},
#ifdef WITH_THREAD
    {"thread", initthread},
#else
    #error no thread!!!
#endif
    {"cStringIO", initcStringIO},
    {"cPickle", initcPickle},
    {"_codecs", init_codecs},
    {"_weakref", init_weakref},
    {"_hotshot", init_hotshot},
    {"_random", init_random},
    {"_bisect", init_bisect},
    {"_lsprof", init_lsprof},
    {"_heapq", init_heapq},
    {"itertools", inititertools},
    {"xxsubtype", initxxsubtype},
    {"zipimport", initzipimport},
    {"_collections", init_collections},
    {"_symtable", init_symtable},
    {"mmap", initmmap},
    {"_csv", init_csv},
    {"_sre", init_sre},
//    {"_ssl", init_ssl},
    {"parser", initparser},
    {"_struct", init_struct},
    {"datetime", initdatetime},
    {"_functools", init_functools},
    {"_json", init_json},
    {"zlib", initzlib},
    {"select", initselect},
    {"_socket", init_socket},
    {"unicodedata", initunicodedata},
    {"_multibytecodec", init_multibytecodec},
    {"_codecs_cn", init_codecs_cn},
    {"_codecs_hk", init_codecs_hk},
    {"_codecs_iso2022", init_codecs_iso2022},
    {"_codecs_jp", init_codecs_jp},
    {"_codecs_kr", init_codecs_kr},
    {"_codecs_tw", init_codecs_tw},


    /* This module lives in marshal.c */
    {"marshal", PyMarshal_Init},

    /* This lives in import.c */
    {"imp", initimp},

    /* These entries are here for sys.builtin_module_names */
    {"__main__", NULL},
    {"__builtin__", NULL},
    {"sys", NULL},
    {"exceptions", NULL},

    /* This lives in gcmodule.c */
    {"gc", initgc},

    {"_io", init_io},

    /* This lives in _warnings.c */
    {"_warnings", _PyWarnings_Init},

    /* Sentinel */
    {0, 0}
};

#ifdef __cplusplus
}
#endif
