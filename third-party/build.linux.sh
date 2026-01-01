set -e
echo "* Building libvterm..."
(cd libvterm && CC=neo-clang CFLAGS="-g3 -Og" make VERBOSE=1)

(cd libxkbcommon && \
 PKG_CONFIG_PATH="`pwd`/../prefix/lib/pkgconfig" CC=neo-clang CXX=neo-clang++ meson setup ../libxkbcommon-meson-build \
 -Dbuildtype=release \
 -Ddefault_library=static \
 -Denable-x11=false \
 -Denable-wayland=false \
 -Denable-xkbregistry=false \
 -Denable-bash-completion=false \
 --prefix=`pwd`/../prefix && \
 meson compile -C ../libxkbcommon-meson-build && \
 meson install -C ../libxkbcommon-meson-build
)

(cd drm && \
 PKG_CONFIG_PATH="`pwd`/../prefix/lib/pkgconfig" CC=neo-clang CXX=neo-clang++ meson setup ../drm-meson-build \
 -Dbuildtype=debug \
 -Ddefault_library=static \
 -Dtests=false \
 --prefix=`pwd`/../prefix && \
 meson compile -C ../drm-meson-build && \
 meson install -C ../drm-meson-build \
)

(cd mesa && \
 CFLAGS="-I`pwd`/../prefix/include" \
 PKG_CONFIG_PATH="`pwd`../prefix/lib/pkgconfig" CC=neo-clang CXX=neo-clang++ meson setup ../mesa-meson-build \
 -Dgles2=enabled \
 -Dplatforms= \
 -Dgallium-drivers= \
 -Dvulkan-drivers= \
 -Dtools= \
 -Dgbm=enabled \
 -Dglx=disabled \
 -Dexpat=disabled \
 -Dzlib=disabled \
 -Dshader-cache=disabled \
 -Ddefault_library=static \
 -Dbuildtype=debug \
 --prefix=`pwd`/../prefix && \
 meson compile -C ../mesa-meson-build && \
 meson install -C ../mesa-meson-build
)

(mkdir -p check-cmake-build && cd check-cmake-build && \
cmake -DCMAKE_INSTALL_PREFIX="`pwd`/../prefix" -DCMAKE_C_COMPILER="neo-clang" -DCMAKE_CXX_COMPILER="neo-clang++" -DCMAKE_SYSROOT=/usr/local/toolchain/aarch64-neobox-linux-musl-sysroot/ ../check &&
cmake --build . --target help && cmake --build . && cmake --install . )

(cd libevdev && \
 PKG_CONFIG_PATH="../prefix/lib/pkgconfig" CC=neo-clang CXX=neo-clang++ meson setup ../libevdev-meson-build -Ddocumentation=disabled --prefix=`pwd`/../prefix && \
 meson compile -C ../libevdev-meson-build && \
 meson install -C ../libevdev-meson-build
 )

(cd libglvnd && \
 CC=neo-clang CXX=neo-clang++ meson setup ../libglvnd-meson-build -Dx11=disabled --prefix=`pwd`/../prefix && \
 meson compile -C ../libglvnd-meson-build && \
 meson install -C ../libglvnd-meson-build
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
