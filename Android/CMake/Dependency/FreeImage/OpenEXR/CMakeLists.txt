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
    ${SRC_ROOT}/OpenEXR
    ${SRC_ROOT}/OpenEXR/IlmImf
    ${SRC_ROOT}/OpenEXR/Imath
    ${SRC_ROOT}/OpenEXR/IexMath
    ${SRC_ROOT}/OpenEXR/Iex
    ${SRC_ROOT}/OpenEXR/Half
    ${SRC_ROOT}/OpenEXR/IlmThread
)

add_library( # Sets the name of the library.
        OpenEXR
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/OpenEXR/IlmImf/b44ExpLogTable.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfAcesFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfB44Compressor.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfBoxAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfChannelList.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfChannelListAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfChromaticities.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfChromaticitiesAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfCompositeDeepScanLine.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfCompressionAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfCompressor.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfConvert.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfCRgbaFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepCompositing.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepFrameBuffer.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepImageStateAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepScanLineInputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepScanLineInputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepScanLineOutputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepScanLineOutputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepTiledInputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepTiledInputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepTiledOutputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDeepTiledOutputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDoubleAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfDwaCompressor.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfEnvmap.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfEnvmapAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfFastHuf.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfFloatAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfFloatVectorAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfFrameBuffer.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfFramesPerSecond.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfGenericInputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfGenericOutputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfHeader.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfHuf.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfInputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfInputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfInputPartData.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfIntAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfIO.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfKeyCode.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfKeyCodeAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfLineOrderAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfLut.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfMatrixAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfMisc.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfMultiPartInputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfMultiPartOutputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfMultiView.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfOpaqueAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfOutputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfOutputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfOutputPartData.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfPartType.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfPizCompressor.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfPreviewImage.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfPreviewImageAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfPxr24Compressor.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfRational.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfRationalAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfRgbaFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfRgbaYca.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfRle.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfRleCompressor.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfScanLineInputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfStandardAttributes.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfStdIO.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfStringAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfStringVectorAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfSystemSpecific.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTestFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfThreading.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTileDescriptionAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTiledInputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTiledInputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTiledMisc.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTiledOutputFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTiledOutputPart.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTiledRgbaFile.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTileOffsets.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTimeCode.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfTimeCodeAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfVecAttribute.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfVersion.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfWav.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfZip.cpp
        ${SRC_ROOT}/OpenEXR/IlmImf/ImfZipCompressor.cpp
        ${SRC_ROOT}/OpenEXR/Imath/ImathBox.cpp
        ${SRC_ROOT}/OpenEXR/Imath/ImathColorAlgo.cpp
        ${SRC_ROOT}/OpenEXR/Imath/ImathFun.cpp
        ${SRC_ROOT}/OpenEXR/Imath/ImathMatrixAlgo.cpp
        ${SRC_ROOT}/OpenEXR/Imath/ImathRandom.cpp
        ${SRC_ROOT}/OpenEXR/Imath/ImathShear.cpp
        ${SRC_ROOT}/OpenEXR/Imath/ImathVec.cpp
        ${SRC_ROOT}/OpenEXR/Iex/IexBaseExc.cpp
        ${SRC_ROOT}/OpenEXR/Iex/IexThrowErrnoExc.cpp
        ${SRC_ROOT}/OpenEXR/Half/half.cpp
        ${SRC_ROOT}/OpenEXR/IlmThread/IlmThread.cpp
        ${SRC_ROOT}/OpenEXR/IlmThread/IlmThreadMutex.cpp
        ${SRC_ROOT}/OpenEXR/IlmThread/IlmThreadPool.cpp
        ${SRC_ROOT}/OpenEXR/IlmThread/IlmThreadSemaphore.cpp
        ${SRC_ROOT}/OpenEXR/IexMath/IexMathFloatExc.cpp
        ${SRC_ROOT}/OpenEXR/IexMath/IexMathFpu.cpp
)

set(CMAKE_CXX_FLAGS -Wno-narrowing)