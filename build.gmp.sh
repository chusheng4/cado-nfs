#!/bin/bash

# Build Parameters #
SRC=/home/cado-nfs
source $SRC/build_params.sh

#gmp version
VGMP=gmp-6.1.0

#get and extract gmp
mkdir -p $SRC/build
rm -rf $SRC/build/$VGMP
rm -f $SRC/build/$VGMP.tar.bz2
cd $SRC/build && wget https://gmplib.org/download/gmp/$VGMP.tar.bz2
cd $SRC/build && tar -xf $VGMP.tar.bz2

if test ${GCCBUILD+defined}; then
echo "GCC BUILD"
BUILD=$SRC/build/gmp/gcc
rm -rf $BUILD
mkdir -p $BUILD
cd $BUILD && $SRC/build/$VGMP/configure --prefix $BUILD
cd $BUILD && make install
else
if test ${MICBUILD+defined}; then
echo "MIC BUILD"
BUILD=$SRC/build/gmp/mic
rm -rf $BUILD
mkdir -p $BUILD
cd $BUILD && $SRC/build/$VGMP/configure CC=icc CFLAGS="-mmic" --host=x86_64 --disable-assembly --prefix $BUILD
cd $BUILD && make install
else
echo "XEON BUILD"
BUILD=$SRC/build/gmp/xeon
rm -rf $BUILD
mkdir -p $BUILD
cd $BUILD && $SRC/build/$VGMP/configure CC=icc CFLAGS="-g -O3" --prefix $BUILD
cd $BUILD && make install
fi
fi
echo Finished
