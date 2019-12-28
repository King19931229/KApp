# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../KRender)

include_directories(
    ${SRC_ROOT}
    ${SRC_ROOT}/../
    ${SRC_ROOT}/../Dependency/glm-0.9.8.0/
    ${SRC_ROOT}/../Dependency/imgui/
    ${ANDROID_NDK}/sources/android/native_app_glue
)

file(GLOB_RECURSE SRC "${SRC_ROOT}/*.cpp" "${SRC_ROOT}/*.h")

add_library(KRender
    STATIC
    ${SRC}
)

target_link_libraries( # Specifies the target library.
    KRender
    KBase
    vulkan
)