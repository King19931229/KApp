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
    ${SRC_ROOT}/LibOpenJPEG
)

add_library( # Sets the name of the library.
        LibOpenJPEG
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/LibOpenJPEG/bio.c
        ${SRC_ROOT}/LibOpenJPEG/cio.c
        ${SRC_ROOT}/LibOpenJPEG/dwt.c
        ${SRC_ROOT}/LibOpenJPEG/event.c
        ${SRC_ROOT}/LibOpenJPEG/function_list.c
        ${SRC_ROOT}/LibOpenJPEG/image.c
        ${SRC_ROOT}/LibOpenJPEG/invert.c
        ${SRC_ROOT}/LibOpenJPEG/j2k.c
        ${SRC_ROOT}/LibOpenJPEG/jp2.c
        ${SRC_ROOT}/LibOpenJPEG/mct.c
        ${SRC_ROOT}/LibOpenJPEG/mqc.c
        ${SRC_ROOT}/LibOpenJPEG/openjpeg.c
        ${SRC_ROOT}/LibOpenJPEG/opj_clock.c
        ${SRC_ROOT}/LibOpenJPEG/pi.c
        ${SRC_ROOT}/LibOpenJPEG/raw.c
        ${SRC_ROOT}/LibOpenJPEG/t1.c
        ${SRC_ROOT}/LibOpenJPEG/t2.c
        ${SRC_ROOT}/LibOpenJPEG/tcd.c
        ${SRC_ROOT}/LibOpenJPEG/tgt.c
)

set(DEFINES
    ANDROID
)

target_compile_definitions(LibOpenJPEG PRIVATE ${DEFINES})