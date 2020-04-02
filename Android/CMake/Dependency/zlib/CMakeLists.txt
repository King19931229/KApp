# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../../Dependency/zlib)

include_directories(
    ${SRC_ROOT}
)

add_library( # Sets the name of the library.
        zlibstatic
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/adler32.c
        ${SRC_ROOT}/compress.c
        ${SRC_ROOT}/crc32.c
        ${SRC_ROOT}/deflate.c
        ${SRC_ROOT}/gzclose.c
        ${SRC_ROOT}/gzlib.c
        ${SRC_ROOT}/gzread.c
        ${SRC_ROOT}/gzwrite.c
        ${SRC_ROOT}/infback.c
        ${SRC_ROOT}/inffast.c
        ${SRC_ROOT}/inflate.c
        ${SRC_ROOT}/inftrees.c
        ${SRC_ROOT}/trees.c
        ${SRC_ROOT}/uncompr.c
        ${SRC_ROOT}/zutil.c
)

set(CMAKE_CXX_FLAGS -Wno-narrowing)