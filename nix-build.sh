#!/usr/bin/env bash
set -e
unset PATH
for p in $buildInputs; do
    export PATH=$p/bin${PATH:+:}$PATH
done
echo $src
ls $src
${CMAKE:-cmake} -B build -DUSE_MPI=ON \
    -DUSE_REPRODUCIBLE=ON -D binarytreesummation_version=47ab47c54b3f32144dbe2a0620d9a3d77588c167 \
    -DCMAKE_BUILD_TYPE=Release -S $src
make -C build
make -C build install
