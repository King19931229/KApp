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
    ${SRC_ROOT}/LibJXR/image/sys
    ${SRC_ROOT}/LibJXR/jxrgluelib
)

add_library( # Sets the name of the library.
        LibJXR
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/LibJXR/image/decode/decode.c
        ${SRC_ROOT}/LibJXR/image/decode/JXRTranscode.c
        ${SRC_ROOT}/LibJXR/image/decode/postprocess.c
        ${SRC_ROOT}/LibJXR/image/decode/segdec.c
        ${SRC_ROOT}/LibJXR/image/decode/strdec.c
        ${SRC_ROOT}/LibJXR/image/decode/strdec_x86.c
        ${SRC_ROOT}/LibJXR/image/decode/strInvTransform.c
        ${SRC_ROOT}/LibJXR/image/decode/strPredQuantDec.c
        ${SRC_ROOT}/LibJXR/image/encode/encode.c
        ${SRC_ROOT}/LibJXR/image/encode/segenc.c
        ${SRC_ROOT}/LibJXR/image/encode/strenc.c
        ${SRC_ROOT}/LibJXR/image/encode/strenc_x86.c
        ${SRC_ROOT}/LibJXR/image/encode/strFwdTransform.c
        ${SRC_ROOT}/LibJXR/image/encode/strPredQuantEnc.c
        ${SRC_ROOT}/LibJXR/image/sys/adapthuff.c
        ${SRC_ROOT}/LibJXR/image/sys/image.c
        ${SRC_ROOT}/LibJXR/image/sys/strcodec.c
        ${SRC_ROOT}/LibJXR/image/sys/strPredQuant.c
        ${SRC_ROOT}/LibJXR/image/sys/strTransform.c
        ${SRC_ROOT}/LibJXR/jxrgluelib/JXRGlue.c
        ${SRC_ROOT}/LibJXR/jxrgluelib/JXRGlueJxr.c
        ${SRC_ROOT}/LibJXR/jxrgluelib/JXRGluePFC.c
        ${SRC_ROOT}/LibJXR/jxrgluelib/JXRMeta.c
)

set(DEFINES
    DISABLE_PERF_MEASUREMENT
    # add __ANSI__ otherwise LibJXR is broken: https://sourceforge.net/p/freeimage/discussion/36110/thread/7e436844/
    __ANSI__
)

target_compile_definitions(LibJXR PRIVATE ${DEFINES})