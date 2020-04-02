# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

set(SRC_ROOT ../../../../../Dependency/assimp-3.3.1)

include_directories(${SRC_ROOT}
    ${SRC_ROOT}/include
    ${SRC_ROOT}/../zlib
    ${SRC_ROOT}/../rapidjson/include
    ${SRC_ROOT}/contrib/openddlparser/include
)

add_library( # Sets the name of the library.
        assimp
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/code/Assimp.cpp
        ${SRC_ROOT}/code/BaseImporter.cpp
        ${SRC_ROOT}/code/BaseProcess.cpp
        ${SRC_ROOT}/code/PostStepRegistry.cpp
        ${SRC_ROOT}/code/ImporterRegistry.cpp
        ${SRC_ROOT}/code/DefaultIOStream.cpp
        ${SRC_ROOT}/code/DefaultIOSystem.cpp
        ${SRC_ROOT}/code/Importer.cpp
        ${SRC_ROOT}/code/SGSpatialSort.cpp
        ${SRC_ROOT}/code/VertexTriangleAdjacency.cpp
        ${SRC_ROOT}/code/SpatialSort.cpp
        ${SRC_ROOT}/code/SceneCombiner.cpp
        ${SRC_ROOT}/code/ScenePreprocessor.cpp
        ${SRC_ROOT}/code/SkeletonMeshBuilder.cpp
        ${SRC_ROOT}/code/SplitByBoneCountProcess.cpp
        ${SRC_ROOT}/code/StandardShapes.cpp
        ${SRC_ROOT}/code/TargetAnimation.cpp
        ${SRC_ROOT}/code/RemoveComments.cpp
        ${SRC_ROOT}/code/Subdivision.cpp
        ${SRC_ROOT}/code/Bitmap.cpp
        ${SRC_ROOT}/code/Version.cpp
        ${SRC_ROOT}/code/DefaultLogger.cpp
        ${SRC_ROOT}/code/Exporter.cpp
        ${SRC_ROOT}/code/AssimpCExport.cpp
        ${SRC_ROOT}/code/CalcTangentsProcess.cpp
        ${SRC_ROOT}/code/ComputeUVMappingProcess.cpp
        ${SRC_ROOT}/code/ConvertToLHProcess.cpp
        ${SRC_ROOT}/code/FindDegenerates.cpp
        ${SRC_ROOT}/code/FindInstancesProcess.cpp
        ${SRC_ROOT}/code/FindInvalidDataProcess.cpp
        ${SRC_ROOT}/code/FixNormalsStep.cpp
        ${SRC_ROOT}/code/GenFaceNormalsProcess.cpp
        ${SRC_ROOT}/code/GenVertexNormalsProcess.cpp
        ${SRC_ROOT}/code/PretransformVertices.cpp
        ${SRC_ROOT}/code/ImproveCacheLocality.cpp
        ${SRC_ROOT}/code/JoinVerticesProcess.cpp
        ${SRC_ROOT}/code/LimitBoneWeightsProcess.cpp
        ${SRC_ROOT}/code/RemoveRedundantMaterials.cpp
        ${SRC_ROOT}/code/RemoveVCProcess.cpp
        ${SRC_ROOT}/code/SortByPTypeProcess.cpp
        ${SRC_ROOT}/code/SplitLargeMeshes.cpp
        ${SRC_ROOT}/code/TextureTransform.cpp
        ${SRC_ROOT}/code/TriangulateProcess.cpp
        ${SRC_ROOT}/code/ValidateDataStructure.cpp
        ${SRC_ROOT}/code/OptimizeGraph.cpp
        ${SRC_ROOT}/code/OptimizeMeshes.cpp
        ${SRC_ROOT}/code/DeboneProcess.cpp
        ${SRC_ROOT}/code/ProcessHelper.cpp
        ${SRC_ROOT}/code/MakeVerboseFormat.cpp
        ${SRC_ROOT}/code/MaterialSystem.cpp
        ${SRC_ROOT}/code/StepExporter.cpp
        ${SRC_ROOT}/code/3DSConverter.cpp
        ${SRC_ROOT}/code/3DSLoader.cpp
        ${SRC_ROOT}/code/3DSExporter.cpp
        ${SRC_ROOT}/code/ACLoader.cpp
        ${SRC_ROOT}/code/ASELoader.cpp
        ${SRC_ROOT}/code/ASEParser.cpp
        ${SRC_ROOT}/code/AssbinExporter.cpp
        ${SRC_ROOT}/code/AssbinLoader.cpp
        ${SRC_ROOT}/code/AssxmlExporter.cpp
        ${SRC_ROOT}/code/B3DImporter.cpp
        ${SRC_ROOT}/code/BVHLoader.cpp
        ${SRC_ROOT}/code/ColladaLoader.cpp
        ${SRC_ROOT}/code/ColladaParser.cpp
        ${SRC_ROOT}/code/ColladaExporter.cpp
        ${SRC_ROOT}/code/DXFLoader.cpp
        ${SRC_ROOT}/code/CSMLoader.cpp
        ${SRC_ROOT}/code/HMPLoader.cpp
        ${SRC_ROOT}/code/IRRMeshLoader.cpp
        ${SRC_ROOT}/code/IRRShared.cpp
        ${SRC_ROOT}/code/IRRLoader.cpp
        ${SRC_ROOT}/code/LWOAnimation.cpp
        ${SRC_ROOT}/code/LWOBLoader.cpp
        ${SRC_ROOT}/code/LWOLoader.cpp
        ${SRC_ROOT}/code/LWOMaterial.cpp
        ${SRC_ROOT}/code/LWSLoader.cpp
        ${SRC_ROOT}/code/MD2Loader.cpp
        ${SRC_ROOT}/code/MD3Loader.cpp
        ${SRC_ROOT}/code/MD5Loader.cpp
        ${SRC_ROOT}/code/MD5Parser.cpp
        ${SRC_ROOT}/code/MDCLoader.cpp
        ${SRC_ROOT}/code/MDLLoader.cpp
        ${SRC_ROOT}/code/MDLMaterialLoader.cpp
        ${SRC_ROOT}/code/NFFLoader.cpp
        ${SRC_ROOT}/code/NDOLoader.cpp
        ${SRC_ROOT}/code/OFFLoader.cpp
        ${SRC_ROOT}/code/ObjFileImporter.cpp
        ${SRC_ROOT}/code/ObjFileMtlImporter.cpp
        ${SRC_ROOT}/code/ObjFileParser.cpp
        ${SRC_ROOT}/code/ObjExporter.cpp
        ${SRC_ROOT}/code/OgreImporter.cpp
        ${SRC_ROOT}/code/OgreStructs.cpp
        ${SRC_ROOT}/code/OgreBinarySerializer.cpp
        ${SRC_ROOT}/code/OgreXmlSerializer.cpp
        ${SRC_ROOT}/code/OgreMaterial.cpp
        ${SRC_ROOT}/code/OpenGEXExporter.cpp
        ${SRC_ROOT}/code/OpenGEXImporter.cpp
        ${SRC_ROOT}/code/PlyLoader.cpp
        ${SRC_ROOT}/code/PlyParser.cpp
        ${SRC_ROOT}/code/PlyExporter.cpp
        ${SRC_ROOT}/code/MS3DLoader.cpp
        ${SRC_ROOT}/code/COBLoader.cpp
        ${SRC_ROOT}/code/BlenderLoader.cpp
        ${SRC_ROOT}/code/BlenderDNA.cpp
        ${SRC_ROOT}/code/BlenderScene.cpp
        ${SRC_ROOT}/code/BlenderModifier.cpp
        ${SRC_ROOT}/code/BlenderBMesh.cpp
        ${SRC_ROOT}/code/BlenderTessellator.cpp
        ${SRC_ROOT}/code/IFCLoader.cpp
        ${SRC_ROOT}/code/IFCReaderGen.cpp
        ${SRC_ROOT}/code/IFCUtil.cpp
        ${SRC_ROOT}/code/IFCGeometry.cpp
        ${SRC_ROOT}/code/IFCMaterial.cpp
        ${SRC_ROOT}/code/IFCProfile.cpp
        ${SRC_ROOT}/code/IFCCurve.cpp
        ${SRC_ROOT}/code/IFCBoolean.cpp
        ${SRC_ROOT}/code/IFCOpenings.cpp
        ${SRC_ROOT}/code/STEPFileReader.cpp
        ${SRC_ROOT}/code/STEPFileEncoding.cpp
        ${SRC_ROOT}/code/XGLLoader.cpp
        ${SRC_ROOT}/code/FBXImporter.cpp
        ${SRC_ROOT}/code/FBXParser.cpp
        ${SRC_ROOT}/code/FBXTokenizer.cpp
        ${SRC_ROOT}/code/FBXConverter.cpp
        ${SRC_ROOT}/code/FBXUtil.cpp
        ${SRC_ROOT}/code/FBXDocument.cpp
        ${SRC_ROOT}/code/FBXProperties.cpp
        ${SRC_ROOT}/code/FBXMeshGeometry.cpp
        ${SRC_ROOT}/code/FBXMaterial.cpp
        ${SRC_ROOT}/code/FBXModel.cpp
        ${SRC_ROOT}/code/FBXAnimation.cpp
        ${SRC_ROOT}/code/FBXNodeAttribute.cpp
        ${SRC_ROOT}/code/FBXDeformer.cpp
        ${SRC_ROOT}/code/FBXBinaryTokenizer.cpp
        ${SRC_ROOT}/code/FBXDocumentUtil.cpp
        ${SRC_ROOT}/code/Q3DLoader.cpp
        ${SRC_ROOT}/code/Q3BSPFileParser.cpp
        ${SRC_ROOT}/code/Q3BSPFileImporter.cpp
        ${SRC_ROOT}/code/Q3BSPZipArchive.cpp
        ${SRC_ROOT}/code/RawLoader.cpp
        ${SRC_ROOT}/code/SIBImporter.cpp
        ${SRC_ROOT}/code/SMDLoader.cpp
        ${SRC_ROOT}/code/STLLoader.cpp
        ${SRC_ROOT}/code/STLExporter.cpp
        ${SRC_ROOT}/code/TerragenLoader.cpp
        ${SRC_ROOT}/code/UnrealLoader.cpp
        ${SRC_ROOT}/code/XFileImporter.cpp
        ${SRC_ROOT}/code/XFileParser.cpp
        ${SRC_ROOT}/code/XFileExporter.cpp
        ${SRC_ROOT}/code/glTFImporter.cpp
        ${SRC_ROOT}/code/glTFExporter.cpp
        ${SRC_ROOT}/code/D3MFImporter.cpp
        ${SRC_ROOT}/code/D3MFOpcPackage.cpp
        ${SRC_ROOT}/contrib/irrXML/irrXML.cpp
        ${SRC_ROOT}/contrib/ConvertUTF/ConvertUTF.c
        ${SRC_ROOT}/contrib/unzip/ioapi.c
        ${SRC_ROOT}/contrib/unzip/unzip.c
        ${SRC_ROOT}/contrib/poly2tri/poly2tri/common/shapes.cc
        ${SRC_ROOT}/contrib/poly2tri/poly2tri/sweep/advancing_front.cc
        ${SRC_ROOT}/contrib/poly2tri/poly2tri/sweep/cdt.cc
        ${SRC_ROOT}/contrib/poly2tri/poly2tri/sweep/sweep.cc
        ${SRC_ROOT}/contrib/poly2tri/poly2tri/sweep/sweep_context.cc
        ${SRC_ROOT}/contrib/clipper/clipper.cpp
        ${SRC_ROOT}/contrib/openddlparser/code/OpenDDLParser.cpp
        ${SRC_ROOT}/contrib/openddlparser/code/DDLNode.cpp
        ${SRC_ROOT}/contrib/openddlparser/code/OpenDDLCommon.cpp
        ${SRC_ROOT}/contrib/openddlparser/code/OpenDDLExport.cpp
        ${SRC_ROOT}/contrib/openddlparser/code/Value.cpp
)

set(DEFINES
    OPENDDL_NO_USE_CPP11
    ASSIMP_BUILD_NO_C4D_IMPORTER
    ASSIMP_BUILD_DLL_EXPORT
    ASSIMP_BUILD_NO_OWN_ZLIB
    OPENDDLPARSER_BUILD
    assimp_EXPORTS
)

set(CMAKE_CXX_FLAGS -Wno-narrowing)

#add_subdirectory(../../zlib ../zlib_binary_dir)

target_compile_definitions(assimp PRIVATE ${DEFINES})

#target_link_libraries(assimp
#        zlibstatic
#)