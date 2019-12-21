# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../../../Dependency/FreeImage)

include_directories(${SRC_ROOT}
    ${SRC_ROOT}/ZLib
)

add_library( # Sets the name of the library.
        ZLib
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/ZLib/adler32.c
        ${SRC_ROOT}/ZLib/compress.c
        ${SRC_ROOT}/ZLib/crc32.c
        ${SRC_ROOT}/ZLib/deflate.c
        ${SRC_ROOT}/ZLib/gzclose.c
        ${SRC_ROOT}/ZLib/gzlib.c
        ${SRC_ROOT}/ZLib/gzread.c
        ${SRC_ROOT}/ZLib/gzwrite.c
        ${SRC_ROOT}/ZLib/infback.c
        ${SRC_ROOT}/ZLib/inffast.c
        ${SRC_ROOT}/ZLib/inflate.c
        ${SRC_ROOT}/ZLib/inftrees.c
        ${SRC_ROOT}/ZLib/trees.c
        ${SRC_ROOT}/ZLib/uncompr.c
        ${SRC_ROOT}/ZLib/zutil.c
)

set(DEFINES
    ANDROID
)

set(CMAKE_CXX_FLAGS -Wno-narrowing)

target_compile_definitions(ZLib PRIVATE ${DEFINES})