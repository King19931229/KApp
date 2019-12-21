# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../../../../Dependency/assimp-3.3.1)

include_directories(${SRC_ROOT}
    ${SRC_ROOT}/contrib
)

add_library( # Sets the name of the library.
        zlibstatic
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/contrib/zlib/adler32.c
        ${SRC_ROOT}/contrib/zlib/compress.c
        ${SRC_ROOT}/contrib/zlib/crc32.c
        ${SRC_ROOT}/contrib/zlib/deflate.c
        ${SRC_ROOT}/contrib/zlib/gzclose.c
        ${SRC_ROOT}/contrib/zlib/gzlib.c
        ${SRC_ROOT}/contrib/zlib/gzread.c
        ${SRC_ROOT}/contrib/zlib/gzwrite.c
        ${SRC_ROOT}/contrib/zlib/infback.c
        ${SRC_ROOT}/contrib/zlib/inffast.c
        ${SRC_ROOT}/contrib/zlib/inflate.c
        ${SRC_ROOT}/contrib/zlib/inftrees.c
        ${SRC_ROOT}/contrib/zlib/trees.c
        ${SRC_ROOT}/contrib/zlib/uncompr.c
        ${SRC_ROOT}/contrib/zlib/zutil.c
)

set(CMAKE_CXX_FLAGS -Wno-narrowing)