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
    ${SRC_ROOT}/LibRawLite
)

add_library( # Sets the name of the library.
        LibRaw
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/LibRawLite/internal/dcraw_common.cpp
        ${SRC_ROOT}/LibRawLite/internal/dcraw_fileio.cpp
        ${SRC_ROOT}/LibRawLite/internal/demosaic_packs.cpp
        ${SRC_ROOT}/LibRawLite/src/libraw_c_api.cpp
        ${SRC_ROOT}/LibRawLite/src/libraw_cxx.cpp
        ${SRC_ROOT}/LibRawLite/src/libraw_datastream.cpp
)

set(DEFINES
    ANDROID
    LIBRAW_NODLL
)

set(CMAKE_CXX_FLAGS -Wno-narrowing)

target_compile_definitions(LibRaw PRIVATE ${DEFINES})