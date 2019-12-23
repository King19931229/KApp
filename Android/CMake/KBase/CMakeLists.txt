# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../KBase)
file(GLOB_RECURSE SRC "${SRC_ROOT}/*.cpp")

include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/../
    ${SRC_ROOT}/../Dependency/FreeImage/
    ${SRC_ROOT}/../Dependency/boost/include/
    ${SRC_ROOT}/../Dependency/Python27/Include/
    ${SRC_ROOT}/../Dependency/Python27/Android/
    ${SRC_ROOT}/../Dependency/assimp-3.3.1/include/
    ${SRC_ROOT}/../Dependency/zip-0.1.16/src/
)

add_subdirectory(../Dependency/imgui imgui_binary_dir)
add_subdirectory(../Dependency/tinyxml2 tinyxml2_binary_dir)
add_subdirectory(../Dependency/zip zip_binary_dir)
add_subdirectory(../Dependency/FreeImage/FreeImage FreeImage_binary_dir)
add_subdirectory(../Dependency/assimp-3.3.1/code code_binary_dir)
add_subdirectory(../Dependency/Python27 Python27_binary_dir)

add_library(KBase
    STATIC
    ${SRC}
)

target_link_libraries( # Specifies the target library.
    KBase
    FreeImage
    python27
    assimp
    imgui
    tinyxml2
    zip
)