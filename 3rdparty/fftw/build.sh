#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=fftw-3.3.10

echo "=== 开始编译FFTW (macOS M1 ARM64) ==="

# 清理之前的构建
rm -rf "$workdir/deploy"
rm -rf "$workdir/${src_dir_inside}"

# 解压源码
echo "解压FFTW源码..."
mkdir -p "$workdir/${src_dir_inside}" && cd "$workdir" && \
tar -zxvf "$workdir/source/${src_dir_inside}.tar.gz"

echo "--- 构建FFTW (ARM64) ---"
############ build FFTW for ARM64 ############
mkdir -p "$workdir/${src_dir_inside}/build" && cd "$workdir/${src_dir_inside}/build"

# 配置CMake，专门针对M1 ARM64
cmake \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_OSX_ARCHITECTURES="arm64" \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
    -DCMAKE_C_FLAGS="-arch arm64 -O3 -march=armv8-a" \
    -DCMAKE_CXX_FLAGS="-arch arm64 -O3 -march=armv8-a" \
    -DBUILD_SHARED_LIBS=OFF \
    -DENABLE_THREADS=ON \
    -DENABLE_FLOAT=ON \
    -DENABLE_SSE=OFF \
    -DENABLE_SSE2=OFF \
    -DENABLE_AVX=OFF \
    -DENABLE_AVX2=OFF \
    -DDISABLE_FORTRAN=ON \
    -DCMAKE_INSTALL_PREFIX="$workdir/deploy" \
    ..

if [[ $? -ne 0 ]]; then
    echo "ERROR: CMake配置失败"
    exit 1
fi

echo "编译FFTW..."
make -j$(sysctl -n hw.ncpu)

if [[ $? -ne 0 ]]; then
    echo "ERROR: 编译失败"
    exit 1
fi

echo "安装FFTW..."
make install

if [[ $? -ne 0 ]]; then
    echo "ERROR: 安装失败"
    exit 1
fi

# 验证架构
echo "验证编译结果..."
file "$workdir/deploy/lib/libfftw3f.a"
lipo -info "$workdir/deploy/lib/libfftw3f.a"

echo "=== FFTW编译成功！==="
echo "库文件位置: $workdir/deploy/lib/"
echo "头文件位置: $workdir/deploy/include/"

# option (BUILD_SHARED_LIBS "Build shared libraries" ON)
# option (BUILD_TESTS "Build tests" ON)

# option (ENABLE_OPENMP "Use OpenMP for multithreading" OFF)
# option (ENABLE_THREADS "Use pthread for multithreading" OFF)
# option (WITH_COMBINED_THREADS "Merge thread library" OFF)

# option (ENABLE_FLOAT "single-precision" OFF)
# option (ENABLE_LONG_DOUBLE "long-double precision" OFF)
# option (ENABLE_QUAD_PRECISION "quadruple-precision" OFF)

# option (ENABLE_SSE "Compile with SSE instruction set support" OFF)
# option (ENABLE_SSE2 "Compile with SSE2 instruction set support" OFF)
# option (ENABLE_AVX "Compile with AVX instruction set support" OFF)
# option (ENABLE_AVX2 "Compile with AVX2 instruction set support" OFF)

# option (DISABLE_FORTRAN "Disable Fortran wrapper routines" OFF)
