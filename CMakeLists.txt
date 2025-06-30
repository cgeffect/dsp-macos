cmake_minimum_required(VERSION 3.26.1)
message("--begin complie---")
project(dsp_example)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-builtins -Wno-deprecated-declarations")

set(CMAKE_C_FLAGS_DEBUG   "-O0 -g")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")

# 设置目标架构为
set(CMAKE_SYSTEM_PROCESSOR "arm64")
set(CMAKE_OSX_ARCHITECTURES "arm64")
set(CMAKE_OSX_DEPLOYMENT_TARGET "14.0" CACHE STRING "Minimum OS X deployment version")
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(libSRV_TRD_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty)
list(APPEND libSRV_INCLUDES_DIR ./)

list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/speexdsp/deploy/include)
list(APPEND libSRV_INCLUDES_DIR ${libSRV_TRD_ROOT_PATH}/rnnoise/deploy/include)

list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/speexdsp/deploy/lib/libspeexdsp.a)
list(APPEND libs_trd_srv ${libSRV_TRD_ROOT_PATH}/rnnoise/deploy/lib/librnnoise.a)

find_library(LZMA_FRAMEWORK lzma)
find_library(LZ_FRAMEWORK z)
find_library(LBZ2_FRAMEWORK bz2)
find_library(LICONV_FRAMEWORK iconv)
set(SYS_LIBS ${LZMA_FRAMEWORK} ${LZ_FRAMEWORK} ${LBZ2_FRAMEWORK} ${LICONV_FRAMEWORK})
find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
list(APPEND SYS_LIBS ${COREFOUNDATION_FRAMEWORK})
find_library(COREVIDEO_FRAMEWORK CoreVideo REQUIRED)
list(APPEND SYS_LIBS ${COREVIDEO_FRAMEWORK})
find_library(VIDEOTOOLBOX VideoToolbox)
list(APPEND SYS_LIBS ${VIDEOTOOLBOX})
find_library(CORE_MEDIA CoreMedia)
list(APPEND SYS_LIBS ${CORE_MEDIA})
find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox)
list(APPEND SYS_LIBS ${AUDIOTOOLBOX_FRAMEWORK})
find_library(SECURITY_FRAMEWORK Security)
list(APPEND SYS_LIBS ${SECURITY_FRAMEWORK})


# 添加源文件
set(SOURCE_FILES    
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/VAD.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/VAD.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/ANS.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util/ANS.cpp
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCE_FILES})

# 只编译vad_example可执行文件
add_executable(vad_example ${CMAKE_CURRENT_SOURCE_DIR}/src/vad_example.cpp ${SOURCE_FILES})
target_include_directories(vad_example PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(vad_example PRIVATE ${libs_trd_srv} ${SYS_LIBS})

# 添加ans_example可执行文件
add_executable(ans_example ${CMAKE_CURRENT_SOURCE_DIR}/src/ans_example.cpp ${SOURCE_FILES})
target_include_directories(ans_example PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(ans_example PRIVATE ${libs_trd_srv} ${SYS_LIBS})

# 添加agc_test可执行文件
add_executable(agc_test ${CMAKE_CURRENT_SOURCE_DIR}/src/agc_test.cpp ${SOURCE_FILES})
target_include_directories(agc_test PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(agc_test PRIVATE ${libs_trd_srv} ${SYS_LIBS})

# 添加rnnoise_test可执行文件
add_executable(rnnoise_test ${CMAKE_CURRENT_SOURCE_DIR}/src/rnnoise_test.cpp)
target_include_directories(rnnoise_test PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(rnnoise_test PRIVATE ${libs_trd_srv} ${SYS_LIBS})

# 添加rnnoise_demo可执行文件
add_executable(rnnoise_demo ${CMAKE_CURRENT_SOURCE_DIR}/src/rnnoise_demo.c)
target_include_directories(rnnoise_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rnnoise/deploy/include)
target_link_libraries(rnnoise_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/rnnoise/deploy/lib/librnnoise.a)

# 添加rnnoise_vad_test可执行文件
add_executable(rnnoise_vad_test ${CMAKE_CURRENT_SOURCE_DIR}/src/rnnoise_vad_test.cpp)
target_include_directories(rnnoise_vad_test PRIVATE ${libSRV_INCLUDES_DIR})
target_link_libraries(rnnoise_vad_test PRIVATE ${libs_trd_srv} ${SYS_LIBS})

# # 设置库路径
# set_target_properties(vad_example PROPERTIES
#     BUILD_WITH_INSTALL_RPATH TRUE
#     INSTALL_RPATH "${CMAKE_SOURCE_DIR}/3rdparty/speexdsp/deploy/lib"
# )

# set_target_properties(ans_example PROPERTIES
#     BUILD_WITH_INSTALL_RPATH TRUE
#     INSTALL_RPATH "${CMAKE_SOURCE_DIR}/3rdparty/speexdsp/deploy/lib"
# )

