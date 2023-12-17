#!/bin/bash
${CMAKE:-cmake} -B build -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DUSE_REPRODUCIBLE=ON \
    -Dbinarytreesummation_version=origin/master \
    -Dipcdebug_version=origin/main \
    -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
make -C build -j$(nproc)
