export OUT=out/static-debug
export ARGS='is_official_build=false skia_use_egl=true skia_use_x11=false skia_use_fontconfig=true skia_use_harfbuzz=true skia_use_system_harfbuzz=false skia_use_icu=true skia_use_system_icu=false' # extra_cflags_cc=["-fsanitize=address,undefined"]'

./build-linux.sh
