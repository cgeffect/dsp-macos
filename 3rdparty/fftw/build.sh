#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=fftw-3.3.10

rm -rf "$workdir/deploy" \
rm -rf "$workdir/${src_dir_inside}" && mkdir -p "$workdir/${src_dir_inside}" && cd "$workdir" && \
tar -zxvf "$workdir/source/${src_dir_inside}.tar.gz"

echo "--- build ${package} ---"
############ build ${package} ############
mkdir -p "$workdir/${src_dir_inside}/build" && cd "$workdir/${src_dir_inside}/build" && \
cmake -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
      -DBUILD_SHARED_LIBS=OFF \
      -DENABLE_THREADS=ON \
      -DENABLE_FLOAT=ON \
      -DENABLE_SSE=ON \
      -DENABLE_SSE2=ON \
      -DENABLE_AVX=ON \
      -DENABLE_AVX2=ON \
      -DDISABLE_FORTRAN=ON \
      -DCMAKE_INSTALL_PREFIX="$workdir/deploy" .. && \
make -j12 && make install

if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build ${package}"
    exit -1
fi

echo "success!"

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
