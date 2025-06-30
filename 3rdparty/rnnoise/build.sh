#!/bin/sh
workdir=$(cd $(dirname $0); pwd)
rootdir=$(cd $workdir/../../; pwd)

src_dir_inside=rnnoise

############ extract source-code ############
echo "--- extract source-code ---"
rm -rf "$workdir/build" && mkdir -p "$workdir/build" && cd "$workdir/build/" && \
tar -zxvf "$workdir/source/${src_dir_inside}.tar.gz"
if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to extract source-code"
    exit -1
fi

############ build zlib ############
rm -rf "${workdir}/deploy"
cd "$workdir/build/${src_dir_inside}"
echo "--- build speexdsp ---"
./autogen.sh && \
./configure \
    --prefix="${workdir}/deploy/" && \
make && make install
if [[ $? -ne 0 ]]; then
    echo "ERROR: Failed to build zlib"
    exit -1
fi

echo "success!"

# Fine tuning of the installation directories:
#   --bindir=DIR            user executables [EPREFIX/bin]
#   --sbindir=DIR           system admin executables [EPREFIX/sbin]
#   --libexecdir=DIR        program executables [EPREFIX/libexec]
#   --sysconfdir=DIR        read-only single-machine data [PREFIX/etc]
#   --sharedstatedir=DIR    modifiable architecture-independent data [PREFIX/com]
#   --localstatedir=DIR     modifiable single-machine data [PREFIX/var]
#   --libdir=DIR            object code libraries [EPREFIX/lib]
#   --includedir=DIR        C header files [PREFIX/include]
#   --oldincludedir=DIR     C header files for non-gcc [/usr/include]
#   --datarootdir=DIR       read-only arch.-independent data root [PREFIX/share]
#   --datadir=DIR           read-only architecture-independent data [DATAROOTDIR]
#   --infodir=DIR           info documentation [DATAROOTDIR/info]
#   --localedir=DIR         locale-dependent data [DATAROOTDIR/locale]
#   --mandir=DIR            man documentation [DATAROOTDIR/man]
#   --docdir=DIR            documentation root [DATAROOTDIR/doc/speexdsp]
#   --htmldir=DIR           html documentation [DOCDIR]
#   --dvidir=DIR            dvi documentation [DOCDIR]
#   --pdfdir=DIR            pdf documentation [DOCDIR]
#   --psdir=DIR             ps documentation [DOCDIR]

# Program names:
#   --program-prefix=PREFIX            prepend PREFIX to installed program names
#   --program-suffix=SUFFIX            append SUFFIX to installed program names
#   --program-transform-name=PROGRAM   run sed PROGRAM on installed program names

# System types:
#   --build=BUILD     configure for building on BUILD [guessed]
#   --host=HOST       cross-compile to build programs to run on HOST [BUILD]

# Optional Features:
#   --disable-option-checking  ignore unrecognized --enable/--with options
#   --disable-FEATURE       do not include FEATURE (same as --enable-FEATURE=no)
#   --enable-FEATURE[=ARG]  include FEATURE [ARG=yes]
#   --enable-silent-rules   less verbose build output (undo: "make V=1")
#   --disable-silent-rules  verbose build output (undo: "make V=0")
#   --disable-maintainer-mode
#                           disable make rules and dependencies not useful (and
#                           sometimes confusing) to the casual installer
#   --enable-shared[=PKGS]  build shared libraries [default=yes]
#   --enable-static[=PKGS]  build static libraries [default=yes]
#   --enable-fast-install[=PKGS]
#                           optimize for fast installation [default=yes]
#   --enable-dependency-tracking
#                           do not reject slow dependency extractors
#   --disable-dependency-tracking
#                           speeds up one-time build
#   --disable-libtool-lock  avoid locking (might break parallel builds)
#   --enable-sse            Enable SSE support
#   --enable-neon           Enable NEON support
#   --enable-fixed-point    Compile as fixed-point
#   --disable-float-api     Disable the floating-point API
#   --disable-examples      Do not build example programs, only the library
#   --enable-arm4-asm       Make use of ARM4 assembly optimizations
#   --enable-arm5e-asm      Make use of ARM5E assembly optimizations
#   --enable-blackfin-asm   Make use of Blackfin assembly optimizations
#   --enable-fixed-point-debug  Debug fixed-point implementation
#   --enable-resample-full-sinc-table Resample full SINC table (no interpolation)
#   --enable-ti-c55x        Enable support for TI C55X DSP

# Optional Packages:
#   --with-PACKAGE[=ARG]    use PACKAGE [ARG=yes]
#   --without-PACKAGE       do not use PACKAGE (same as --with-PACKAGE=no)
#   --with-pic[=PKGS]       try to use only PIC/non-PIC objects [default=use
#                           both]
#   --with-gnu-ld           assume the C compiler uses GNU ld [default=no]
#   --with-sysroot=DIR Search for dependent libraries within DIR
#                         (or the compiler's sysroot if not specified).
#   --with-fft=choice       use an alternate FFT implementation. The available
#                           choices are kiss (default fixed point), smallft
#                           (default floating point), gpl-fftw3 and
#                           proprietary-intel-mkl


# https://blog.csdn.net/weixin_40355471/article/details/126198285