#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON \
    -DUSE_REPRODUCIBLE=ON -D binarytreesummation_version=929f8456dd8ab1ea15b16d102038c8d1505c2e26 \
    -DCMAKE_BUILD_TYPE=Release -S .
make -C build -j$(nproc)
