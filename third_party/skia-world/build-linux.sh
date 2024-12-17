mkdir -p ${OUT}

cd skia

# Need to run /Applications/Python\ 3.10/Install\ Certificates.command before starting

#python3 bin/fetch-gn
#python3 tools/git-sync-deps
#python3 bin/fetch-ninja

echo bin/gn gen ../${OUT} --args="${ARGS}"
bin/gn gen ../${OUT} --args="${ARGS}"
ninja -C ../${OUT} -t compdb > ../compile_commands.json

ninja -C ../${OUT}/ -t targets all | grep -o "lib[a-z]*\.a" > ../${OUT}/lib_targets.txt
sort -u ../${OUT}/lib_targets.txt > ../${OUT}/lib_targets_nodupes.txt

ninja -C ../${OUT} $(cat ../${OUT}/lib_targets_nodupes.txt)


#for lib_target in $(cat ../out/Static/lib_targets_nodupes.txt)
#do
#  echo "Building the library ${lib_target}"
#  ninja -C ../out/Static ${lib_target}
#done

#ninja -C ../out/Static -j 1     # Linking the remaining (executables) with only 1 core to avoid OOM

#--args="is_official_build=true skia_use_system_expat=false skia_use_system_icu=false skia_use_libjpeg_turbo=false skia_use_system_libpng=false skia_use_system_libwebp=false skia_use_system_zlib=false skia_use_libwebp=false extra_cflags_cc=[\"-frtti\"]"
