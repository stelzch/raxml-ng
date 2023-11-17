#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON \
    -DUSE_REPRODUCIBLE=ON -D binarytreesummation_version=063eb8b5c455ba0b22f52cecb941d6e7820e2571 \
    -DCMAKE_BUILD_TYPE=Release -S .
make -C build -j$(nproc)
