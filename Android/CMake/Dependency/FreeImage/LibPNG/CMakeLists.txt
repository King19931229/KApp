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
    ${SRC_ROOT}/LibPNG
)

add_library( # Sets the name of the library.
        LibPNG
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/LibPNG/png.c
        ${SRC_ROOT}/LibPNG/pngerror.c
        ${SRC_ROOT}/LibPNG/pngget.c
        ${SRC_ROOT}/LibPNG/pngmem.c
        ${SRC_ROOT}/LibPNG/pngpread.c
        ${SRC_ROOT}/LibPNG/pngread.c
        ${SRC_ROOT}/LibPNG/pngrio.c
        ${SRC_ROOT}/LibPNG/pngrtran.c
        ${SRC_ROOT}/LibPNG/pngrutil.c
        ${SRC_ROOT}/LibPNG/pngset.c
        ${SRC_ROOT}/LibPNG/pngtrans.c
        ${SRC_ROOT}/LibPNG/pngwio.c
        ${SRC_ROOT}/LibPNG/pngwrite.c
        ${SRC_ROOT}/LibPNG/pngwtran.c
        ${SRC_ROOT}/LibPNG/pngwutil.c
)

set(DEFINES
    ANDROID
)

target_compile_definitions(LibPNG PRIVATE ${DEFINES})