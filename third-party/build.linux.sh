set -e
cd skia-world
source env.sh
./build-linux.debug.sh
./build-linux.release.sh
cd ..

mkdir -p prefix/lib-debug
mkdir -p prefix/lib-release
mkdir -p prefix/include/skia/

cp_dir() {
#    pwd
#    echo ---- d1 --- "$1"
#    echo ---- d2 --- "$2"
    dn2=$(dirname "$2")
    abs1=$(realpath -s --relative-to="$dn2" .)
#    echo ---- abs1 --- "$abs1"
    trg="$abs1"/"$1"
#    echo ---- trg --- "$trg"
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