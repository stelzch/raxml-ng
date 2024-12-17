#!/bin/bash
binarytreesummation_version=c08203d8faffc22664daaa908bd133f2221c1a40
ipcdebug_version=06ab8697e1ffe22904488866de2cc057e7a78393
${CMAKE:-cmake} -B build -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DREPRODUCIBLE=REPROBLAS \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-nonrepr -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
make -C build -j$(nproc)
make -C build-nonrepr -j$(nproc)
