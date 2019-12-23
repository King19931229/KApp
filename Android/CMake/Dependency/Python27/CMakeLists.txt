# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../../Dependency/Python27)

include_directories(${SRC_ROOT}
    ${SRC_ROOT}/Include
    ${SRC_ROOT}/Python
    ${SRC_ROOT}/Modules/zlib
    ${SRC_ROOT}/Android
)

add_library( # Sets the name of the library.
        python27
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/Modules/arraymodule.c
        ${SRC_ROOT}/Modules/audioop.c
        ${SRC_ROOT}/Modules/binascii.c
        ${SRC_ROOT}/Modules/cjkcodecs/multibytecodec.c
        ${SRC_ROOT}/Modules/cjkcodecs/_codecs_cn.c
        ${SRC_ROOT}/Modules/cjkcodecs/_codecs_hk.c
        ${SRC_ROOT}/Modules/cjkcodecs/_codecs_iso2022.c
        ${SRC_ROOT}/Modules/cjkcodecs/_codecs_jp.c
        ${SRC_ROOT}/Modules/cjkcodecs/_codecs_kr.c
        ${SRC_ROOT}/Modules/cjkcodecs/_codecs_tw.c
        ${SRC_ROOT}/Modules/cmathmodule.c
        ${SRC_ROOT}/Modules/cPickle.c
        ${SRC_ROOT}/Modules/cStringIO.c
        ${SRC_ROOT}/Modules/datetimemodule.c
        ${SRC_ROOT}/Modules/errnomodule.c
        ${SRC_ROOT}/Modules/future_builtins.c
        ${SRC_ROOT}/Modules/gcmodule.c
        ${SRC_ROOT}/Modules/getbuildinfo.c

        ${SRC_ROOT}/Modules/posixmodule.c
#        ${SRC_ROOT}/Modules/_ssl.c
        ${SRC_ROOT}/Modules/posixmodule.c
        ${SRC_ROOT}/Modules/selectmodule.c
        ${SRC_ROOT}/Modules/socketmodule.c

        ${SRC_ROOT}/Modules/getpath.c
        ${SRC_ROOT}/Modules/pwdmodule.c
        ${SRC_ROOT}/Modules/fcntlmodule.c
        ${SRC_ROOT}/Modules/unicodedata.c

        ${SRC_ROOT}/Modules/imageop.c
        ${SRC_ROOT}/Modules/itertoolsmodule.c
        ${SRC_ROOT}/Modules/main.c
        ${SRC_ROOT}/Modules/mathmodule.c
        ${SRC_ROOT}/Modules/md5.c
        ${SRC_ROOT}/Modules/md5module.c
        ${SRC_ROOT}/Modules/mmapmodule.c
        ${SRC_ROOT}/Modules/operator.c
        ${SRC_ROOT}/Modules/parsermodule.c
        ${SRC_ROOT}/Modules/rotatingtree.c
        ${SRC_ROOT}/Modules/sha256module.c
        ${SRC_ROOT}/Modules/sha512module.c
        ${SRC_ROOT}/Modules/shamodule.c
        ${SRC_ROOT}/Modules/signalmodule.c
        ${SRC_ROOT}/Modules/stropmodule.c
        ${SRC_ROOT}/Modules/symtablemodule.c
        ${SRC_ROOT}/Modules/threadmodule.c
        ${SRC_ROOT}/Modules/timemodule.c
        ${SRC_ROOT}/Modules/xxsubtype.c
        ${SRC_ROOT}/Modules/zipimport.c
        ${SRC_ROOT}/Modules/zlibmodule.c
        ${SRC_ROOT}/Modules/zlib/adler32.c
        ${SRC_ROOT}/Modules/zlib/compress.c
        ${SRC_ROOT}/Modules/zlib/crc32.c
        ${SRC_ROOT}/Modules/zlib/deflate.c
        ${SRC_ROOT}/Modules/zlib/gzclose.c
        ${SRC_ROOT}/Modules/zlib/gzlib.c
        ${SRC_ROOT}/Modules/zlib/gzread.c
        ${SRC_ROOT}/Modules/zlib/gzwrite.c
        ${SRC_ROOT}/Modules/zlib/infback.c
        ${SRC_ROOT}/Modules/zlib/inffast.c
        ${SRC_ROOT}/Modules/zlib/inflate.c
        ${SRC_ROOT}/Modules/zlib/inftrees.c
        ${SRC_ROOT}/Modules/zlib/trees.c
        ${SRC_ROOT}/Modules/zlib/uncompr.c
        ${SRC_ROOT}/Modules/zlib/zutil.c
        ${SRC_ROOT}/Modules/_bisectmodule.c
        ${SRC_ROOT}/Modules/_codecsmodule.c
        ${SRC_ROOT}/Modules/_collectionsmodule.c
        ${SRC_ROOT}/Modules/_csv.c
        ${SRC_ROOT}/Modules/_functoolsmodule.c
        ${SRC_ROOT}/Modules/_heapqmodule.c
        ${SRC_ROOT}/Modules/_hotshot.c
        ${SRC_ROOT}/Modules/_io/bufferedio.c
        ${SRC_ROOT}/Modules/_io/bytesio.c
        ${SRC_ROOT}/Modules/_io/fileio.c
        ${SRC_ROOT}/Modules/_io/iobase.c
        ${SRC_ROOT}/Modules/_io/stringio.c
        ${SRC_ROOT}/Modules/_io/textio.c
        ${SRC_ROOT}/Modules/_io/_iomodule.c
        ${SRC_ROOT}/Modules/_json.c
        ${SRC_ROOT}/Modules/_localemodule.c
        ${SRC_ROOT}/Modules/_lsprof.c
        ${SRC_ROOT}/Modules/_math.c
        ${SRC_ROOT}/Modules/_randommodule.c
        ${SRC_ROOT}/Modules/_sre.c
        ${SRC_ROOT}/Modules/_struct.c
        ${SRC_ROOT}/Modules/_weakref.c
        ${SRC_ROOT}/Objects/abstract.c
        ${SRC_ROOT}/Objects/boolobject.c
        ${SRC_ROOT}/Objects/bufferobject.c
        ${SRC_ROOT}/Objects/bytearrayobject.c
        ${SRC_ROOT}/Objects/bytes_methods.c
        ${SRC_ROOT}/Objects/capsule.c
        ${SRC_ROOT}/Objects/cellobject.c
        ${SRC_ROOT}/Objects/classobject.c
        ${SRC_ROOT}/Objects/cobject.c
        ${SRC_ROOT}/Objects/codeobject.c
        ${SRC_ROOT}/Objects/complexobject.c
        ${SRC_ROOT}/Objects/descrobject.c
        ${SRC_ROOT}/Objects/dictobject.c
        ${SRC_ROOT}/Objects/enumobject.c
        ${SRC_ROOT}/Objects/exceptions.c
        ${SRC_ROOT}/Objects/fileobject.c
        ${SRC_ROOT}/Objects/floatobject.c
        ${SRC_ROOT}/Objects/frameobject.c
        ${SRC_ROOT}/Objects/funcobject.c
        ${SRC_ROOT}/Objects/genobject.c
        ${SRC_ROOT}/Objects/intobject.c
        ${SRC_ROOT}/Objects/iterobject.c
        ${SRC_ROOT}/Objects/listobject.c
        ${SRC_ROOT}/Objects/longobject.c
        ${SRC_ROOT}/Objects/memoryobject.c
        ${SRC_ROOT}/Objects/methodobject.c
        ${SRC_ROOT}/Objects/moduleobject.c
        ${SRC_ROOT}/Objects/object.c
        ${SRC_ROOT}/Objects/obmalloc.c
        ${SRC_ROOT}/Objects/rangeobject.c
        ${SRC_ROOT}/Objects/setobject.c
        ${SRC_ROOT}/Objects/sliceobject.c
        ${SRC_ROOT}/Objects/stringobject.c
        ${SRC_ROOT}/Objects/structseq.c
        ${SRC_ROOT}/Objects/tupleobject.c
        ${SRC_ROOT}/Objects/typeobject.c
        ${SRC_ROOT}/Objects/unicodectype.c
        ${SRC_ROOT}/Objects/unicodeobject.c
        ${SRC_ROOT}/Objects/weakrefobject.c
        ${SRC_ROOT}/Parser/acceler.c
        ${SRC_ROOT}/Parser/bitset.c
        ${SRC_ROOT}/Parser/firstsets.c
        ${SRC_ROOT}/Parser/grammar.c
        ${SRC_ROOT}/Parser/grammar1.c
        ${SRC_ROOT}/Parser/listnode.c
        ${SRC_ROOT}/Parser/metagrammar.c
        ${SRC_ROOT}/Parser/myreadline.c
        ${SRC_ROOT}/Parser/node.c
        ${SRC_ROOT}/Parser/parser.c
        ${SRC_ROOT}/Parser/parsetok.c
        ${SRC_ROOT}/Parser/tokenizer.c
        ${SRC_ROOT}/Android/config.c
        ${SRC_ROOT}/Python/asdl.c
        ${SRC_ROOT}/Python/ast.c
        ${SRC_ROOT}/Python/bltinmodule.c
        ${SRC_ROOT}/Python/ceval.c
        ${SRC_ROOT}/Python/codecs.c
        ${SRC_ROOT}/Python/compile.c
        ${SRC_ROOT}/Python/dtoa.c
#        ${SRC_ROOT}/Python/dynload_win.c
        ${SRC_ROOT}/Python/errors.c
        ${SRC_ROOT}/Python/formatter_string.c
        ${SRC_ROOT}/Python/formatter_unicode.c
        ${SRC_ROOT}/Python/frozen.c
        ${SRC_ROOT}/Python/future.c
        ${SRC_ROOT}/Python/getargs.c
        ${SRC_ROOT}/Python/getcompiler.c
        ${SRC_ROOT}/Python/getcopyright.c
        ${SRC_ROOT}/Python/getopt.c
        ${SRC_ROOT}/Python/getplatform.c
        ${SRC_ROOT}/Python/getversion.c
        ${SRC_ROOT}/Python/graminit.c
        ${SRC_ROOT}/Python/import.c
        ${SRC_ROOT}/Python/importdl.c
        ${SRC_ROOT}/Python/marshal.c
        ${SRC_ROOT}/Python/modsupport.c
        ${SRC_ROOT}/Python/mysnprintf.c
        ${SRC_ROOT}/Python/mystrtoul.c
        ${SRC_ROOT}/Python/peephole.c
        ${SRC_ROOT}/Python/pyarena.c
        ${SRC_ROOT}/Python/pyctype.c
        ${SRC_ROOT}/Python/pyfpe.c
        ${SRC_ROOT}/Python/pymath.c
        ${SRC_ROOT}/Python/pystate.c
        ${SRC_ROOT}/Python/pystrcmp.c
        ${SRC_ROOT}/Python/pystrtod.c
        ${SRC_ROOT}/Python/Python-ast.c
        ${SRC_ROOT}/Python/pythonrun.c
        ${SRC_ROOT}/Python/random.c
        ${SRC_ROOT}/Python/structmember.c
        ${SRC_ROOT}/Python/symtable.c
        ${SRC_ROOT}/Python/sysmodule.c
        ${SRC_ROOT}/Python/thread.c
        ${SRC_ROOT}/Python/traceback.c
        ${SRC_ROOT}/Python/_warnings.c
)

set(DEFINES
    Py_ENABLE_SHARED
    Py_BUILD_CORE
)

target_compile_definitions(python27 PRIVATE ${DEFINES})