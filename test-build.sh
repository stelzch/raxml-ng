#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON \
    -DUSE_REPRODUCIBLE=ON -D binarytreesummation_version=4d3f3a98918458bf6e7fb5c0be4bc36cccb2e51e \
    -DCMAKE_BUILD_TYPE=Release -S .
make -C build -j$(nproc)
