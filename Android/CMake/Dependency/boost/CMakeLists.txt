# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../../Dependency/boost)

include_directories(
        ${SRC_ROOT}
        ${SRC_ROOT}/include/
        ${SRC_ROOT}/../Python27/Android
        ${SRC_ROOT}/../Python27/Include
)

add_library( # Sets the name of the library.
        boost
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/src/converter/arg_to_python_base.cpp
        ${SRC_ROOT}/src/converter/builtin_converters.cpp
        ${SRC_ROOT}/src/converter/from_python.cpp
        ${SRC_ROOT}/src/converter/registry.cpp
        ${SRC_ROOT}/src/converter/type_id.cpp
        ${SRC_ROOT}/src/dict.cpp
        ${SRC_ROOT}/src/errors.cpp
        ${SRC_ROOT}/src/exec.cpp
        ${SRC_ROOT}/src/import.cpp
        ${SRC_ROOT}/src/list.cpp
        ${SRC_ROOT}/src/long.cpp
        ${SRC_ROOT}/src/module.cpp
        ${SRC_ROOT}/src/numpy/dtype.cpp
        ${SRC_ROOT}/src/numpy/matrix.cpp
        ${SRC_ROOT}/src/numpy/ndarray.cpp
        ${SRC_ROOT}/src/numpy/numpy.cpp
        ${SRC_ROOT}/src/numpy/scalars.cpp
        ${SRC_ROOT}/src/numpy/ufunc.cpp
        ${SRC_ROOT}/src/object/class.cpp
        ${SRC_ROOT}/src/object/enum.cpp
        ${SRC_ROOT}/src/object/function.cpp
        ${SRC_ROOT}/src/object/function_doc_signature.cpp
        ${SRC_ROOT}/src/object/inheritance.cpp
        ${SRC_ROOT}/src/object/iterator.cpp
        ${SRC_ROOT}/src/object/life_support.cpp
        ${SRC_ROOT}/src/object/pickle_support.cpp
        ${SRC_ROOT}/src/object/stl_iterator.cpp
        ${SRC_ROOT}/src/object_operators.cpp
        ${SRC_ROOT}/src/object_protocol.cpp
        ${SRC_ROOT}/src/slice.cpp
        ${SRC_ROOT}/src/str.cpp
        ${SRC_ROOT}/src/tuple.cpp
        ${SRC_ROOT}/src/wrapper.cpp
)

set(DEFINES
    BOOSTPYTHON_EXPORTS
    BOOST_PYTHON_SOURCE
    BOOST_NUMPY_SOURCE
)

target_compile_definitions(boost PRIVATE ${DEFINES})