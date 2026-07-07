#!/bin/bash
set -e

echo "ARCH = "${ARCH:=x86_64}
echo "BUILD_TYPE = "${BUILD_TYPE:=release}
echo ""
#echo "ARCH set to ${ARCH}"
echo "* Building libvterm..."

# for neo-clang
export PKG_CONFIG_PATH="/usr/local/toolchain/${ARCH}-neobox-linux-musl-toolchain/share/pkgconfig/:/usr/local/toolchain/${ARCH}-neobox-linux-musl-toolchain/lib/pkgconfig/:`pwd`/prefix/lib/${ARCH}-linux-musl/pkgconfig/:`pwd`/prefix/lib/pkgconfig/"
export PKG_CONFIG_SYSROOT_DIR="/usr/local/toolchain/${ARCH}-neobox-linux-musl-toolchain"
export LLVM_CONFIG="/usr/local/toolchain/${ARCH}-neobox-linux-musl-toolchain/bin/llvm-config"

# for system clang
#export PKG_CONFIG_PATH="/usr/lib/pkgconfig:/usr/lib/aarch64-linux-gnu/pkgconfig:`pwd`/prefix/lib/${ARCH}-linux-musl/pkgconfig/:`pwd`/prefix/lib/pkgconfig/"
#export PKG_CONFIG_SYSROOT_DIR="/"

export MESON="`pwd`/meson/meson.py"
export MESON_PKG_CONFIG=1
export CC=neo-clang
export CXX=neo-clang++

#(cd libvterm && CC=neo-clang CFLAGS="-g3 -Og" make VERBOSE=1 DEBUG=1)
(cd libvterm && CFLAGS="-g0 -O3" make VERBOSE=1)

(cd libxkbcommon && \
  $MESON setup --prefer-static ../libxkbcommon-meson-build \
 -Dbuildtype=${BUILD_TYPE} \
 -Ddefault_library=static \
 -Denable-x11=false \
 -Denable-wayland=false \
 -Denable-xkbregistry=false \
 -Denable-bash-completion=false \
 --prefix=`pwd`/../prefix && \
 $MESON compile -C ../libxkbcommon-meson-build && \
 $MESON install -C ../libxkbcommon-meson-build
)

if [[ "$ARCH" == "x86_64" ]]
then
  (cd libpciaccess && \
    $MESON setup --prefer-static ../libpciaccess-meson-build \
   -Dbuildtype=${BUILD_TYPE} \
   -Dzlib=disabled \
   -Ddefault_library=static \
   --prefix=`pwd`/../prefix && \
   $MESON compile -C ../libpciaccess-meson-build && \
   $MESON install -C ../libpciaccess-meson-build \
  )
fi

(cd drm && \
  $MESON setup --prefer-static ../drm-meson-build \
 -Dbuildtype=${BUILD_TYPE} \
 -Ddefault_library=static \
 -Dtests=false \
 --prefix=`pwd`/../prefix && \
 $MESON compile -C ../drm-meson-build && \
 $MESON install -C ../drm-meson-build \
)

# (cd libglvnd && \
#   $MESON setup --prefer-static ../libglvnd-meson-build -Dbuildtype=${BUILD_TYPE} -Dx11=disabled --prefix=`pwd`/../prefix && \
#  $MESON compile -C ../libglvnd-meson-build && \
#  $MESON install -C ../libglvnd-meson-build
#  )

# i915,r300,r600,radeonsi,v3d,vc4,freedreno,crocus,etnaviv,   ,rocket,ethosu
# -Dvulkan-drivers='panfrost' \
#  -Dgallium-drivers='nouveau,svga,tegra,virgl,lima,panfrost,llvmpipe,iris,zink,asahi' \

(cd mesa && \
 CC=$CC CXX=$CXX CFLAGS="-I`pwd`/../prefix/include -I`pwd`/../prefix/include/libdrm -mno-outline-atomics" \
 CXXFLAGS="-I`pwd`/../prefix/include -I`pwd`/../prefix/include/libdrm -mno-outline-atomics" \
 LDFLAGS="-L`pwd`/../prefix/lib -Wl,--undefined-version -Wl,--allow-shlib-undefined" \
  $MESON setup --prefer-static ../mesa-meson-build \
 --cross-file `pwd`/../neobox-meson-cross.txt \
 -Dbuildtype=${BUILD_TYPE} \
 -Degl=enabled \
 -Dgles2=enabled \
 -Dplatforms= \
 -Dgallium-drivers='panfrost' \
 \
 -Dvulkan-drivers='' \
 -Dunversion-libgallium=true \
 -Dtools= \
 -Dgbm=enabled \
 -Dglx=disabled \
 -Dexpat=disabled \
 -Dzlib=disabled \
 -Dshader-cache=disabled \
 -Ddefault_library=static \
 -Dshared-llvm=disabled \
 -Db_lundef=false \
 -Dgbm-backends-path="<dirname>:/usr/lib/${ARCH}-linux-musl/gbm:/usr/lib/${ARCH}-linux-gnu/gbm" \
 --libdir="lib/${ARCH}-linux-musl" \
 --prefix=`pwd`/../prefix && \
 $MESON compile -C ../mesa-meson-build && \
 $MESON install -C ../mesa-meson-build
)

(mkdir -p check-cmake-build && cd check-cmake-build && \
cmake -DCMAKE_INSTALL_PREFIX="`pwd`/../prefix" -DCMAKE_C_COMPILER="$CC" -DCMAKE_CXX_COMPILER="$CXX" -DCMAKE_SYSROOT="${PKG_CONFIG_SYSROOT_DIR}" ../check &&
cmake --build . --target help && cmake --build . && cmake --install . )

(cd libevdev && \
 CFLAGS="-I`pwd`/../prefix/include" \
 LDFLAGS="-L`pwd`/../prefix/lib" \
  $MESON setup --prefer-static ../libevdev-meson-build -Dbuildtype=${BUILD_TYPE} -Ddocumentation=disabled --prefix=`pwd`/../prefix && \
 $MESON compile -C ../libevdev-meson-build && \
 $MESON install -C ../libevdev-meson-build
)

(cd libexecinfo && \
 make DESTDIR=`pwd`/../prefix install-static install-headers)


echo "* Building skia..."
(cd skia-world && \
source env.sh && \
./build-linux.debug.sh && \
./build-linux.release.sh \
)

mkdir -p prefix/lib-debug
mkdir -p prefix/lib-release
mkdir -p prefix/include/skia/

cp_dir() {
    dn2=$(dirname "$2")
    abs1=$(realpath -s --relative-to="$dn2" .)
    trg="$abs1"/"$1"
    mkdir -p $(dirname "$2") && ln -fs "$trg" "$2"
}
export -f cp_dir

cd skia-world/out/static-debug
find . -name "*.a" \
         -exec echo "Symlinking {} to prefix/lib-debug/{}" \; \
         -exec bash -c 'cp_dir "{}" "../../../prefix/lib-debug/{}"' \;

cd ../static-release
find . -name "*.a" \
         -exec echo "Symlinking {} to prefix/lib-release/{}" \; \
         -exec bash -c 'cp_dir "{}" "../../../prefix/lib-release/{}"' \;

cd ../../skia
find include -name *.h \
         -exec echo "Symlinking {} to prefix/include/skia/{}" \; \
         -exec bash -c 'cp_dir "{}" "../../prefix/include/skia/{}"' \;

find modules -name *.h \
         -exec echo "Symlinking {} to prefix/include/skia/{}" \; \
         -exec bash -c 'cp_dir "{}" "../../prefix/include/skia/{}"' \;

find src -name *.h \
         -exec echo "Symlinking {} to prefix/include/skia/{}" \; \
         -exec bash -c 'cp_dir "{}" "../../prefix/include/skia/{}"' \;

cd third_party/externals
find icu -name *.h \
         -exec echo "Symlinking {} to prefix/include/{}" \; \
         -exec bash -c 'cp_dir "{}" "../../../../prefix/include/skia/{}"' \;

#third-party/skia-world/skia/third_party/externals/icu/source/common/unicode/stringpiece.h
