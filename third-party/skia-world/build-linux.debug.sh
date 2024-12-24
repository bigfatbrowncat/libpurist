export OUT=out/static-debug
export ARGS='is_official_build=false skia_use_egl=true skia_use_x11=false skia_use_fontconfig=true' # extra_cflags_cc=["-fsanitize=address,undefined"]'

./build-linux.sh
