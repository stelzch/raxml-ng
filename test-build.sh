#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON \
    -DUSE_REPRODUCIBLE=ON -D binarytreesummation_version=47ab47c54b3f32144dbe2a0620d9a3d77588c167 \
    -DCMAKE_BUILD_TYPE=Release -S .
make -C build -j$(nproc)
