#!/bin/bash
# Export factor trials data to timestamped file
#   Support multiple builds

# Define a timestamp function
timestamp() {
	date +"%Y%m%d%H%M%S"
}

# Build Parameters #
SRC=/home/cado-nfs
source $SRC/build_params.sh
trials=/mnt/nfs/System/trials

if test ${GCCBUILD+defined}; then
echo "GCC BUILD"
BUILDTREE=$SRC/build/gcc
PYTHON3=python3
filename=$trials/E5-2603v3-GCC-$(timestamp).txt
else
if test ${MICBUILD+defined}; then
echo "MIC BUILD"
BUILDTREE=$SRC/build/mic
PYTHON3="ssh mic0 /home/Python/release/Python-3.4.4-mic/bin/python3"
filename=$trials/MIC5110P-$(timestamp).txt
else
echo "XEON BUILD"
BUILDTREE=$SRC/build/intel64
PYTHON3=python3
filename=$trials/E5-2603v3-ICC-$(timestamp).txt
fi
fi

echo "Export simple factor trials data to timestamped file: $filename"
# Simple Test Routine #
$PYTHON3 $BUILDTREE/cado-nfs.py N=90377629292003121684002147101760858109247336549001090677693 slaves.nrclients=6 tasks.threads=2 2>&1 | tee $filename
