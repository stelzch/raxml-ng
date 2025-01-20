#!/bin/bash
binarytreesummation_version=561d28783713c99d80d9bca65680166026e0358e
ipcdebug_version=06ab8697e1ffe22904488866de2cc057e7a78393
${CMAKE:-cmake} -B build -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DREPRODUCIBLE=ON \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
#${CMAKE:-cmake} -B build-nonrepr -DUSE_MPI=ON -DBUILD_TESTS=OFF \
#    -Dbinarytreesummation_version=$binarytreesummation_version \
#    -Dipcdebug_version=$ipcdebug_version \
#    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
#cmake --build build --parallel
#cmake --build build-nonrepr --parallel
