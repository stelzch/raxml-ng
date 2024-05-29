#!/bin/bash
nproc=$1
simd=$2
variant="build-$3"
if [ -n "$nproc" ] && [ "$nproc" -eq "$nproc" ] 2> /dev/null; then
    echo Using $nproc processors 
else
    echo Must specify number of cores as first argument.
    exit 1
fi
if [[ -z "$2" ]]; then
    echo Must specify SIMD set as first argument: none, sse, avx, avx2
    exit 1
fi
if [[ -z "$3" ]]; then
    variant="build-nonrepr"
fi
out=R_partial_avx2_${PREFIX}_p${nproc} 
echo Prefix is $out, build folder $variant
mpirun \
    --use-hwthread-cpus \
	-np $nproc $variant/bin/raxml-ng-mpi --prefix $out \
	--redo \
    --log DEBUG --precision 50\
	--seed 42 \
	--msa ../test-Datasets/DNA-Data/354/354.phy \
	--pat-comp off \
    --site-repeats off \
    --tip-inner off \
    --simd $simd \
	--workers 1 \
    --threads 1 \
	--model GTR+G \
	--tree rand{1}
