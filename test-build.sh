#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DUSE_REPRODUCIBLE=ON \
    -Dbinarytreesummation_version=fbb5e8d7d24eedfcfb4b5650b4213da9ba86e1b5 \
    -Dipcdebug_version=8cf2664687d175037f3b42fd4359dde2ec4a50e3 \
    -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
make -C build -j$(nproc)
