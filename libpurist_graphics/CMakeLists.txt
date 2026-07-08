project(purist_graphics)

add_library(purist_graphics STATIC
    # Public includes
    include/purist/graphics/Display.h
    include/purist/graphics/DisplayContentsHandler.h
    include/purist/graphics/FrameBuffer.h
    include/purist/graphics/Mode.h
    include/purist/graphics/TargetSurface.h
    include/purist/graphics/GPUProvider.h

    # Private includes/sources
    src/purist/graphics/Card.cpp
    src/purist/graphics/Card.h
    src/purist/graphics/DisplayImpl.cpp
    src/purist/graphics/DisplayImpl.h
    src/purist/graphics/Displays.cpp
    src/purist/graphics/Displays.h
    src/purist/graphics/DumbBufferTargetSurfaceImpl.cpp
    src/purist/graphics/DumbBufferTargetSurfaceImpl.h
    src/purist/graphics/EGLTargetSurfaceImpl.cpp
    src/purist/graphics/EGLTargetSurfaceImpl.h
    src/purist/graphics/DumbBufferMapping.cpp
    src/purist/graphics/DumbBufferMapping.h
    src/purist/graphics/FrameBufferImpl.cpp
    src/purist/graphics/FrameBufferImpl.h
    src/purist/graphics/ModeConnector.cpp
    src/purist/graphics/ModeConnector.h
    src/purist/graphics/ModeModeInfo.cpp
    src/purist/graphics/ModeModeInfo.h
    src/purist/graphics/ModeCrtc.cpp
    src/purist/graphics/ModeCrtc.h
    src/purist/graphics/ModeEncoder.cpp
    src/purist/graphics/ModeEncoder.h
    src/purist/graphics/ModeResources.cpp
    src/purist/graphics/ModeResources.h
    src/purist/graphics/TargetSurfaceBackface.h
)

find_package(PkgConfig REQUIRED)
#pkg_search_module(EVDEV REQUIRED libevdev)
#pkg_search_module(DRM REQUIRED libdrm)
#pkg_search_module(GBM REQUIRED gbm)
#pkg_search_module(XKB_COMMON REQUIRED xkbcommon)
#pkg_search_module(GLES2 REQUIRED glesv2)
#pkg_search_module(GLVND REQUIRED libglvnd)

target_compile_features(purist_graphics PUBLIC cxx_std_17)
set_property(TARGET purist_graphics PROPERTY POSITION_INDEPENDENT_CODE ON)
target_include_directories(purist_graphics PRIVATE ${DRM_INCLUDE_DIRS})
target_include_directories(purist_graphics PRIVATE ${GBM_INCLUDE_DIRS})
#target_include_directories(purist_graphics PRIVATE ${EVDEV_INCLUDE_DIRS})
#target_include_directories(purist_graphics PRIVATE ${XKB_COMMON_INCLUDE_DIRS})
target_include_directories(purist_graphics PUBLIC ${GLES2_INCLUDE_DIRS})

#target_include_directories(purist_graphics PUBLIC ${purist_third_party}/evdevw/include)

# Publishing the include dir
target_include_directories(purist_graphics PUBLIC ${purist_graphics_SOURCE_DIR}/include)

# Using includes from libpurist_platform
target_include_directories(purist_graphics PUBLIC ${purist_platform_SOURCE_DIR}/include)


target_include_directories(purist_graphics PUBLIC ${purist_third_party}/prefix/include)
target_include_directories(purist_graphics PUBLIC ${purist_third_party}/prefix/include/libdrm)  # TODO Remove this when the third party components are ordered
target_link_directories(purist_graphics PUBLIC ${purist_third_party}/prefix/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-musl)
target_link_directories(purist_graphics PUBLIC ${purist_third_party}/prefix/lib)

target_link_options(purist_graphics PRIVATE "-Wl,-z,muldefs")

target_link_libraries(purist_graphics PUBLIC 
    evdev
    xkbcommon

    
    drm 
    pciaccess
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,drm_intel>
    gbm 
    #GLdispatch  # from glvnd
    #GLESv2      # from glvnd

    #EGL_mesa

    #-Wl,-Bdynamic
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/prefix/lib/aarch64-linux-musl/libgallium_dri.so>
    #-Wl,-Bstatic

    ${purist_third_party}/prefix/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-musl/libgallium_dri.a
    $<LINK_LIBRARY:WHOLE_ARCHIVE,${purist_third_party}/prefix/lib/${CMAKE_SYSTEM_PROCESSOR}-linux-musl/gbm/dri_gbm.a>


    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/mesa/libmesa.a>

    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/compiler/nir/libnir.a>
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/compiler/libcompiler.a>
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/compiler/glsl/libglsl.a>
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/compiler/glsl/libglsl_util.a>
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/gallium/auxiliary/pipe-loader/libpipe_loader_static.a>

    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/gallium/auxiliary/libgallium.a>

    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/mesa/glapi/shared-glapi/libglapi.a>
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/gallium/frontends/dri/libdri.a>
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/mesa-meson-build/src/gallium/winsys/sw/kms-dri/libswkmsdri.a>
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,EGL_mesa>

    EGL         # from glvnd
    #GLdispatch
    GLESv2

    SPIRV-Tools
    #$<LINK_LIBRARY:WHOLE_ARCHIVE,/home/il/projects/libpurist/third-party/prefix/lib/aarch64-linux-musl/gbm/dri_gbm.a>
    #/home/il/projects/libpurist/third-party/prefix/lib/aarch64-linux-musl/gbm/dri_gbm.a
    
    -Wl,--allow-multiple-definition
    #-Wl,-u,gbmint_get_backend
    
    
    #/home/il/projects/libpurist/third-party/prefix/lib/aarch64-linux-musl/libgallium.a
    
    LLVMCore
    LLVMSupport
    LLVMAArch64Desc
    LLVMX86Desc
    LLVMMC
    LLVMTargetParser
    LLVMAArch64Utils
    LLVMRuntimeDyld
    LLVMObject
    LLVMBitReader
    LLVMTextAPI
    LLVMBinaryFormat
    LLVMDemangle
    LLVMTarget
    LLVMRemarks
    LLVMMCJIT
    LLVMAArch64CodeGen
    LLVMX86CodeGen
    LLVMCodeGen
    LLVMAnalysis
    LLVMTransformUtils
    LLVMProfileData
    LLVMGlobalISel
    LLVMDebugInfoDWARF
    LLVMCodeGenTypes
    LLVMAsmPrinter
    LLVMDebugInfoCodeView
    LLVMDebugInfoDWARFLowLevel
    LLVMCGData
    LLVMMCParser
    LLVMScalarOpts
    LLVMSelectionDAG
    LLVMExecutionEngine
    LLVMAArch64Info
    LLVMX86Info
    LLVMPasses
    LLVMInstrumentation
    LLVMipo
    LLVMFrontendOpenMP
    LLVMLinker
    LLVMVectorize
    LLVMSandboxIR
    LLVMOrcTargetProcess
    LLVMIRPrinter
    LLVMFrontendAtomic
    LLVMCoroutines
    LLVMHipStdPar
    LLVMObjCARCOpts
    LLVMCFGuard
    LLVMBitstreamReader
    LLVMAArch64Disassembler
    LLVMX86Disassembler
    LLVMBitWriter
    LLVMIRReader
    LLVMMCDisassembler
    LLVMInstCombine
    LLVMFrontendOffloading
    LLVMAggressiveInstCombine
    LLVMObjectYAML
    LLVMAsmParser
)

# Installing the library
install(TARGETS purist_graphics DESTINATION lib-$<IF:$<CONFIG:Debug>,debug,release>)
install(DIRECTORY include/ DESTINATION include)
