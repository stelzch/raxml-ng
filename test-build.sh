#!/bin/bash
binarytreesummation_version=a144cddc6d474b150a58fac6750fb7a319e75af7
ipcdebug_version=8cf2664687d175037f3b42fd4359dde2ec4a50e3
${CMAKE:-cmake} -B build-bts -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DREPRODUCIBLE=BINARYTREESUMMATION \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-reproblas -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DREPRODUCIBLE=REPROBLAS \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-nonrepr -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
for folder in build-bts build-reproblas build-nonrepr
do
    make -C $folder -j$(nproc)
done
