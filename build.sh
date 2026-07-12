(mkdir -p build-Debug && cd build-Debug && \
cmake \
      -DCMAKE_C_COMPILER=neo-clang \
      -DCMAKE_CXX_COMPILER=neo-clang++ \
      -DCMAKE_BUILD_TYPE=Debug .. && cmake --build . -j 4)

(mkdir -p build-Release && cd build-Release && \
cmake \
      -DCMAKE_C_COMPILER=neo-clang \
      -DCMAKE_CXX_COMPILER=neo-clang++ \
      -DCMAKE_BUILD_TYPE=Release .. && cmake --build . -j 4)

#env PKG_CONFIG_PATH=../third-party/prefix/lib/pkgconfig/ cmake -DCMAKE_C_COMPILER=neo-clang -DCMAKE_CXX_COMPILER=neo-clang++ -DCMAKE_SYSROOT=/usr/local/toolchain/aarch64-neobox-linux-musl-sysroot/ .. && cmake --build .
#env PKG_CONFIG_PATH="`pwd`/third-party/prefix/lib/pkgconfig/:`pwd`/third-party/prefix/lib/aarch64-linux-musl/pkgconfig/" cmake \
#env PKG_CONFIG_PATH="`pwd`/third-party/prefix/lib/aarch64-linux-musl/pkgconfig/" cmake \

