#!/bin/bash
binarytreesummation_version=fbb5e8d7d24eedfcfb4b5650b4213da9ba86e1b5 
ipcdebug_version=8cf2664687d175037f3b42fd4359dde2ec4a50e3
${CMAKE:-cmake} -B build-bts -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DUSE_REPRODUCIBLE=BINARYTREESUMMATION \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-reproblas -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DUSE_REPRODUCIBLE=REPROBLAS \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-nonrepr -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DFETCHCONTENT_QUIET=OFF \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
for folder in build-bts build-reproblas build-nonrepr
do
    make -C $folder -j$(nproc)
done
