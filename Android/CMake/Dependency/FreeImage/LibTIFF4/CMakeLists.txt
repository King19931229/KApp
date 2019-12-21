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
    ${SRC_ROOT}/LibTIFF4
)

add_library( # Sets the name of the library.
        LibTIFF4
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/LibTIFF4/tif_aux.c
        ${SRC_ROOT}/LibTIFF4/tif_close.c
        ${SRC_ROOT}/LibTIFF4/tif_codec.c
        ${SRC_ROOT}/LibTIFF4/tif_color.c
        ${SRC_ROOT}/LibTIFF4/tif_compress.c
        ${SRC_ROOT}/LibTIFF4/tif_dir.c
        ${SRC_ROOT}/LibTIFF4/tif_dirinfo.c
        ${SRC_ROOT}/LibTIFF4/tif_dirread.c
        ${SRC_ROOT}/LibTIFF4/tif_dirwrite.c
        ${SRC_ROOT}/LibTIFF4/tif_dumpmode.c
        ${SRC_ROOT}/LibTIFF4/tif_error.c
        ${SRC_ROOT}/LibTIFF4/tif_extension.c
        ${SRC_ROOT}/LibTIFF4/tif_fax3.c
        ${SRC_ROOT}/LibTIFF4/tif_fax3sm.c
        ${SRC_ROOT}/LibTIFF4/tif_flush.c
        ${SRC_ROOT}/LibTIFF4/tif_getimage.c
        ${SRC_ROOT}/LibTIFF4/tif_jpeg.c
        ${SRC_ROOT}/LibTIFF4/tif_luv.c
        ${SRC_ROOT}/LibTIFF4/tif_lzma.c
        ${SRC_ROOT}/LibTIFF4/tif_lzw.c
        ${SRC_ROOT}/LibTIFF4/tif_next.c
        ${SRC_ROOT}/LibTIFF4/tif_ojpeg.c
        ${SRC_ROOT}/LibTIFF4/tif_open.c
        ${SRC_ROOT}/LibTIFF4/tif_packbits.c
        ${SRC_ROOT}/LibTIFF4/tif_pixarlog.c
        ${SRC_ROOT}/LibTIFF4/tif_predict.c
        ${SRC_ROOT}/LibTIFF4/tif_print.c
        ${SRC_ROOT}/LibTIFF4/tif_read.c
        ${SRC_ROOT}/LibTIFF4/tif_strip.c
        ${SRC_ROOT}/LibTIFF4/tif_swab.c
        ${SRC_ROOT}/LibTIFF4/tif_thunder.c
        ${SRC_ROOT}/LibTIFF4/tif_tile.c
        ${SRC_ROOT}/LibTIFF4/tif_version.c
        ${SRC_ROOT}/LibTIFF4/tif_warning.c
        ${SRC_ROOT}/LibTIFF4/tif_write.c
        ${SRC_ROOT}/LibTIFF4/tif_zip.c
)

set(DEFINES
    ANDROID
)

set(CMAKE_CXX_FLAGS -Wno-narrowing)

target_compile_definitions(LibTIFF4 PRIVATE ${DEFINES})