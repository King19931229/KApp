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
    ${SRC_ROOT}/DeprecationManager
    ${SRC_ROOT}/OpenEXR
    ${SRC_ROOT}/OpenEXR/Half
    ${SRC_ROOT}/OpenEXR/Iex
    ${SRC_ROOT}/OpenEXR/IlmImf
    ${SRC_ROOT}/OpenEXR/Imath
    ${SRC_ROOT}/OpenEXR/IlmThread
    ${SRC_ROOT}/LibJXR/jxrgluelib
    ${SRC_ROOT}/LibJXR/image/sys
    ${SRC_ROOT}/LibJXR/common/include
)

add_library( # Sets the name of the library.
        FreeImage
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/FreeImage/BitmapAccess.cpp
        ${SRC_ROOT}/FreeImage/ColorLookup.cpp
        ${SRC_ROOT}/FreeImage/FreeImage.cpp
        ${SRC_ROOT}/FreeImage/FreeImageIO.cpp
        ${SRC_ROOT}/FreeImage/GetType.cpp
        ${SRC_ROOT}/FreeImage/MemoryIO.cpp
        ${SRC_ROOT}/FreeImage/PixelAccess.cpp
        ${SRC_ROOT}/FreeImage/LFPQuantizer.cpp
        ${SRC_ROOT}/FreeImage/NNQuantizer.cpp
        ${SRC_ROOT}/FreeImage/WuQuantizer.cpp
        ${SRC_ROOT}/FreeImage/Conversion.cpp
        ${SRC_ROOT}/FreeImage/Conversion16_555.cpp
        ${SRC_ROOT}/FreeImage/Conversion16_565.cpp
        ${SRC_ROOT}/FreeImage/Conversion24.cpp
        ${SRC_ROOT}/FreeImage/Conversion32.cpp
        ${SRC_ROOT}/FreeImage/Conversion4.cpp
        ${SRC_ROOT}/FreeImage/Conversion8.cpp
        ${SRC_ROOT}/FreeImage/ConversionFloat.cpp
        ${SRC_ROOT}/FreeImage/ConversionRGB16.cpp
        ${SRC_ROOT}/FreeImage/ConversionRGBA16.cpp
        ${SRC_ROOT}/FreeImage/ConversionRGBAF.cpp
        ${SRC_ROOT}/FreeImage/ConversionRGBF.cpp
        ${SRC_ROOT}/FreeImage/ConversionType.cpp
        ${SRC_ROOT}/FreeImage/ConversionUINT16.cpp
        ${SRC_ROOT}/FreeImage/Halftoning.cpp
        ${SRC_ROOT}/FreeImage/tmoColorConvert.cpp
        ${SRC_ROOT}/FreeImage/tmoDrago03.cpp
        ${SRC_ROOT}/FreeImage/tmoFattal02.cpp
        ${SRC_ROOT}/FreeImage/tmoReinhard05.cpp
        ${SRC_ROOT}/FreeImage/ToneMapping.cpp
        ${SRC_ROOT}/FreeImage/J2KHelper.cpp
        ${SRC_ROOT}/FreeImage/MNGHelper.cpp
        ${SRC_ROOT}/FreeImage/Plugin.cpp
        ${SRC_ROOT}/FreeImage/PluginBMP.cpp
        ${SRC_ROOT}/FreeImage/PluginCUT.cpp
        ${SRC_ROOT}/FreeImage/PluginDDS.cpp
        ${SRC_ROOT}/FreeImage/PluginEXR.cpp
        ${SRC_ROOT}/FreeImage/PluginG3.cpp
        ${SRC_ROOT}/FreeImage/PluginGIF.cpp
        ${SRC_ROOT}/FreeImage/PluginHDR.cpp
        ${SRC_ROOT}/FreeImage/PluginICO.cpp
        ${SRC_ROOT}/FreeImage/PluginIFF.cpp
        ${SRC_ROOT}/FreeImage/PluginJ2K.cpp
        ${SRC_ROOT}/FreeImage/PluginJNG.cpp
        ${SRC_ROOT}/FreeImage/PluginJP2.cpp
        ${SRC_ROOT}/FreeImage/PluginJPEG.cpp
        ${SRC_ROOT}/FreeImage/PluginJXR.cpp
        ${SRC_ROOT}/FreeImage/PluginKOALA.cpp
        ${SRC_ROOT}/FreeImage/PluginMNG.cpp
        ${SRC_ROOT}/FreeImage/PluginPCD.cpp
        ${SRC_ROOT}/FreeImage/PluginPCX.cpp
        ${SRC_ROOT}/FreeImage/PluginPFM.cpp
        ${SRC_ROOT}/FreeImage/PluginPICT.cpp
        ${SRC_ROOT}/FreeImage/PluginPNG.cpp
        ${SRC_ROOT}/FreeImage/PluginPNM.cpp
        ${SRC_ROOT}/FreeImage/PluginPSD.cpp
        ${SRC_ROOT}/FreeImage/PluginRAS.cpp
        ${SRC_ROOT}/FreeImage/PluginRAW.cpp
        ${SRC_ROOT}/FreeImage/PluginSGI.cpp
        ${SRC_ROOT}/FreeImage/PluginTARGA.cpp
        ${SRC_ROOT}/FreeImage/PluginTIFF.cpp
        ${SRC_ROOT}/FreeImage/PluginWBMP.cpp
        ${SRC_ROOT}/FreeImage/PluginWebP.cpp
        ${SRC_ROOT}/FreeImage/PluginXBM.cpp
        ${SRC_ROOT}/FreeImage/PluginXPM.cpp
        ${SRC_ROOT}/FreeImage/PSDParser.cpp
        ${SRC_ROOT}/FreeImage/TIFFLogLuv.cpp
        ${SRC_ROOT}/DeprecationManager/Deprecated.cpp
        ${SRC_ROOT}/DeprecationManager/DeprecationMgr.cpp
        ${SRC_ROOT}/FreeImage/CacheFile.cpp
        ${SRC_ROOT}/FreeImage/MultiPage.cpp
        ${SRC_ROOT}/FreeImage/ZLibInterface.cpp
        ${SRC_ROOT}/Metadata/Exif.cpp
        ${SRC_ROOT}/Metadata/FIRational.cpp
        ${SRC_ROOT}/Metadata/FreeImageTag.cpp
        ${SRC_ROOT}/Metadata/IPTC.cpp
        ${SRC_ROOT}/Metadata/TagConversion.cpp
        ${SRC_ROOT}/Metadata/TagLib.cpp
        ${SRC_ROOT}/Metadata/XTIFF.cpp
        ${SRC_ROOT}/FreeImageToolkit/Background.cpp
        ${SRC_ROOT}/FreeImageToolkit/BSplineRotate.cpp
        ${SRC_ROOT}/FreeImageToolkit/Channels.cpp
        ${SRC_ROOT}/FreeImageToolkit/ClassicRotate.cpp
        ${SRC_ROOT}/FreeImageToolkit/Colors.cpp
        ${SRC_ROOT}/FreeImageToolkit/CopyPaste.cpp
        ${SRC_ROOT}/FreeImageToolkit/Display.cpp
        ${SRC_ROOT}/FreeImageToolkit/Flip.cpp
        ${SRC_ROOT}/FreeImageToolkit/JPEGTransform.cpp
        ${SRC_ROOT}/FreeImageToolkit/MultigridPoissonSolver.cpp
        ${SRC_ROOT}/FreeImageToolkit/Rescale.cpp
        ${SRC_ROOT}/FreeImageToolkit/Resize.cpp
)

set(DEFINES
    FREEIMAGE_COLORORDER=FREEIMAGE_COLORORDER_RGB
    OPJ_STATIC
    FREEIMAGE_LIB
    LIBRAW_NODLL
    # add __ANSI__ otherwise LibJXR is broken: https://sourceforge.net/p/freeimage/discussion/36110/thread/7e436844/
    __ANSI__
)
target_compile_definitions(FreeImage PRIVATE ${DEFINES})

add_subdirectory(../LibJPEG LibJPEG_binary_dir)
add_subdirectory(../LibJXR LibJXR_binary_dir)
add_subdirectory(../LibPNG LibPNG_binary_dir)
add_subdirectory(../LibOpenJPEG LibOpenJPEG_binary_dir)
add_subdirectory(../LibRawLite LibRawLite_binary_dir)
add_subdirectory(../LibTIFF4 LibTIFF4_binary_dir)
add_subdirectory(../LibWebP LibWebP_binary_dir)
add_subdirectory(../OpenEXR OpenEXR_binary_dir)
add_subdirectory(../ZLib ZLib_binary_dir)

target_link_libraries(FreeImage
    LibJPEG
    LibJXR
    LibPNG
    LibOpenJPEG
    LibRaw
    LibTIFF4
    LibWebP
    OpenEXR
    ZLib
)