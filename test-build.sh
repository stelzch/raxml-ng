#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON \
    -DUSE_REPRODUCIBLE=ON -D binarytreesummation_version=c167806f096f2e556787b62b5414bdb71a573358 \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
make -C build -j$(nproc)
