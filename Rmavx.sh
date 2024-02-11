#!/bin/bash
nproc=$1
if [ -n "$nproc" ] && [ "$nproc" -eq "$nproc" ] 2> /dev/null; then
    echo Using $nproc processors 
else
    echo Must specify number of cores as first argument.
    exit 1
fi
out=R_partial_avx2_${PREFIX}_p${nproc} 
echo Prefix is $out
mpirun \
    --use-hwthread-cpus \
	-np $nproc build/bin/raxml-ng-mpi --prefix $out \
	--redo \
    --log DEBUG --precision 50\
	--seed 42 \
	--msa ../test-Datasets/DNA-Data/354/354.phy \
	--pat-comp off \
    --site-repeats off \
    --tip-inner off \
    --simd avx2 \
	--workers 1 \
	--model GTR+G \
	--tree rand{1}
