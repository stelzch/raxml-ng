#!/bin/bash
binarytreesummation_version=0cfd6def34684a9fbbd3ea534cc04db91ea34a2f
ipcdebug_version=06ab8697e1ffe22904488866de2cc057e7a78393
${CMAKE:-cmake} -B build-bts -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DREPRODUCIBLE=BINARYTREESUMMATION \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-bts-debug -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DREPRODUCIBLE=BINARYTREESUMMATION \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DPLLMOD_DEBUG=NO -DPLL_DEBUG=NO -DIPCDEBUG=1 \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
    #-DPLLMOD_DEBUG=YES -DPLL_DEBUG=YES -DIPCDEBUG=1 \
${CMAKE:-cmake} -B build-reproblas -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -DREPRODUCIBLE=REPROBLAS \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-nonrepr -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
${CMAKE:-cmake} -B build-nonrepr-debug -DUSE_MPI=ON -DBUILD_TESTS=OFF \
    -Dbinarytreesummation_version=$binarytreesummation_version \
    -Dipcdebug_version=$ipcdebug_version \
    -DPLLMOD_DEBUG=YES -DPLL_DEBUG=YES \
    -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=YES -S .
for folder in build-bts build-reproblas build-nonrepr
do
    #make -C $folder -j$(nproc)
done
