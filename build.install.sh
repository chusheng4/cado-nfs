#!/bin/bash
# Build Parameters #
source /home/cado-nfs/build_params.sh
SRC=/home/cado-nfs

if test ${GCCBUILD+defined}; then
echo "GCC BUILD"
BUILDTREE=$SRC/build/gcc
else
if test ${MICBUILD+defined}; then
echo "MIC BUILD"
BUILDTREE=$SRC/build/mic
else
echo "XEON BUILD"
BUILDTREE=$SRC/build/intel64
fi
fi

rm -rf $BUILDTREE
mkdir -p $BUILDTREE
cd $SRC && make -j12 install
sh $SRC/build.trial.sh
