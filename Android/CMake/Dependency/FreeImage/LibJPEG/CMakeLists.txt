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
        LibJPEG
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/LibJPEG/jaricom.c
        ${SRC_ROOT}/LibJPEG/jcapimin.c
        ${SRC_ROOT}/LibJPEG/jcapistd.c
        ${SRC_ROOT}/LibJPEG/jcarith.c
        ${SRC_ROOT}/LibJPEG/jccoefct.c
        ${SRC_ROOT}/LibJPEG/jccolor.c
        ${SRC_ROOT}/LibJPEG/jcdctmgr.c
        ${SRC_ROOT}/LibJPEG/jchuff.c
        ${SRC_ROOT}/LibJPEG/jcinit.c
        ${SRC_ROOT}/LibJPEG/jcmainct.c
        ${SRC_ROOT}/LibJPEG/jcmarker.c
        ${SRC_ROOT}/LibJPEG/jcmaster.c
        ${SRC_ROOT}/LibJPEG/jcomapi.c
        ${SRC_ROOT}/LibJPEG/jcparam.c
        ${SRC_ROOT}/LibJPEG/jcprepct.c
        ${SRC_ROOT}/LibJPEG/jcsample.c
        ${SRC_ROOT}/LibJPEG/jctrans.c
        ${SRC_ROOT}/LibJPEG/jdapimin.c
        ${SRC_ROOT}/LibJPEG/jdapistd.c
        ${SRC_ROOT}/LibJPEG/jdarith.c
        ${SRC_ROOT}/LibJPEG/jdatadst.c
        ${SRC_ROOT}/LibJPEG/jdatasrc.c
        ${SRC_ROOT}/LibJPEG/jdcoefct.c
        ${SRC_ROOT}/LibJPEG/jdcolor.c
        ${SRC_ROOT}/LibJPEG/jddctmgr.c
        ${SRC_ROOT}/LibJPEG/jdhuff.c
        ${SRC_ROOT}/LibJPEG/jdinput.c
        ${SRC_ROOT}/LibJPEG/jdmainct.c
        ${SRC_ROOT}/LibJPEG/jdmarker.c
        ${SRC_ROOT}/LibJPEG/jdmaster.c
        ${SRC_ROOT}/LibJPEG/jdmerge.c
        ${SRC_ROOT}/LibJPEG/jdpostct.c
        ${SRC_ROOT}/LibJPEG/jdsample.c
        ${SRC_ROOT}/LibJPEG/jdtrans.c
        ${SRC_ROOT}/LibJPEG/jerror.c
        ${SRC_ROOT}/LibJPEG/jfdctflt.c
        ${SRC_ROOT}/LibJPEG/jfdctfst.c
        ${SRC_ROOT}/LibJPEG/jfdctint.c
        ${SRC_ROOT}/LibJPEG/jidctflt.c
        ${SRC_ROOT}/LibJPEG/jidctfst.c
        ${SRC_ROOT}/LibJPEG/jidctint.c
        ${SRC_ROOT}/LibJPEG/jmemmgr.c
        ${SRC_ROOT}/LibJPEG/jmemnobs.c
        ${SRC_ROOT}/LibJPEG/jquant1.c
        ${SRC_ROOT}/LibJPEG/jquant2.c
        ${SRC_ROOT}/LibJPEG/jutils.c
        ${SRC_ROOT}/LibJPEG/transupp.c
)

set(DEFINES
    ANDROID
)

target_compile_definitions(LibJPEG PRIVATE ${DEFINES})