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
    ${SRC_ROOT}/LibWebP
    ${ANDROID_NDK}/sources/android/cpufeatures
)

add_library( # Sets the name of the library.
        LibWebP
        # Sets the library as a static library.
        STATIC
        # Provides a relative path to your source file(s).
        ${SRC_ROOT}/LibWebP/src/dec/dec.alpha.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.buffer.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.frame.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.idec.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.io.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.quant.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.tree.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.vp8.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.vp8l.c
        ${SRC_ROOT}/LibWebP/src/dec/dec.webp.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.alpha_processing.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.alpha_processing_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.alpha_processing_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.argb.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.argb_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.argb_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.cost.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.cost_mips32.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.cost_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.cost_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.cpu.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.dec.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.dec_clip_tables.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.dec_mips32.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.dec_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.dec_neon.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.dec_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.enc.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.enc_avx2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.enc_mips32.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.enc_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.enc_neon.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.enc_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.filters.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.filters_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.filters_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.lossless.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.lossless_mips32.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.lossless_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.lossless_neon.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.lossless_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.rescaler.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.rescaler_mips32.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.rescaler_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.upsampling.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.upsampling_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.upsampling_neon.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.upsampling_sse2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.yuv.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.yuv_mips32.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.yuv_mips_dsp_r2.c
        ${SRC_ROOT}/LibWebP/src/dsp/dsp.yuv_sse2.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.alpha.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.analysis.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.backward_references.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.config.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.cost.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.filter.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.frame.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.histogram.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.iterator.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.near_lossless.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.picture.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.picture_csp.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.picture_psnr.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.picture_rescale.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.picture_tools.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.quant.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.syntax.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.token.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.tree.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.vp8l.c
        ${SRC_ROOT}/LibWebP/src/enc/enc.webpenc.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.bit_reader.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.bit_writer.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.color_cache.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.filters.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.huffman.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.huffman_encode.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.quant_levels.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.quant_levels_dec.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.random.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.rescaler.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.thread.c
        ${SRC_ROOT}/LibWebP/src/utils/utils.utils.c
        ${SRC_ROOT}/LibWebP/src/mux/mux.anim_encode.c
        ${SRC_ROOT}/LibWebP/src/mux/mux.muxedit.c
        ${SRC_ROOT}/LibWebP/src/mux/mux.muxinternal.c
        ${SRC_ROOT}/LibWebP/src/mux/mux.muxread.c
        ${SRC_ROOT}/LibWebP/src/demux/demux.demux.c
)

add_library(cpufeatures ${ANDROID_NDK}/sources/android/cpufeatures/cpu-features.c)

set(CMAKE_CXX_FLAGS -Wno-narrowing)

target_link_libraries(LibWebP
    cpufeatures
)