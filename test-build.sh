#!/bin/bash
cmake -B build -DUSE_MPI=ON -DUSE_REPRODUCIBLE=ON -DCMAKE_BUILD_TYPE=Release -S .
make -C build -j$(nproc)
