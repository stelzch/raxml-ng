#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DUSE_REPRODUCIBLE=ON \
    -Dbinarytreesummation_version=fbb5e8d7d24eedfcfb4b5650b4213da9ba86e1b5 \
    -Dipcdebug_version=06f9c7c4e18ef5d1406c65332650cad0ce6b22c7 \
    -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
make -C build -j$(nproc)
