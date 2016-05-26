#!/bin/bash

# Build Systems #
SRC=/home/cado-nfs
BUILDPARAMS=$SRC/build_params.sh
source $SRC/build_systems.sh

if test ${MICTOOLS+defined}; then
# MIC Build #
rm -rf $BUILDPARAMS
cat > $BUILDPARAMS <<EOF
#!/bin/bash
#GCCBUILD=1
MICBUILD=1
EOF
# GMP requirement
GMPlibrary=$SRC/build/gmp/mic/lib/libgmp.a
if [ -f $GMPlibrary ];
then
echo "$GMPlibrary found"
else
echo "Building $GMPlibrary"
cd $SRC && sh $SRC/build.gmp.sh
fi
cd $SRC && sh $SRC/build.sh
else
echo "MICTOOLS disabled"
fi

if test ${GCCTOOLS+defined}; then
# GCC Build #
rm -rf $BUILDPARAMS
cat > $BUILDPARAMS <<EOF
#!/bin/bash
GCCBUILD=1
#MICBUILD=1
EOF
# GMP requirement
GMPlibrary=$SRC/build/gmp/gcc/lib/libgmp.a
if [ -f $GMPlibrary ];
then
echo "$GMPlibrary found"
else
echo "Building $GMPlibrary"
cd $SRC && sh $SRC/build.gmp.sh
fi
cd $SRC && sh $SRC/build.sh
else
echo "GCCTOOLS disabled"
fi

if test ${XEONTOOLS+defined}; then
# ICC Build #
rm -rf $BUILDPARAMS
cat > $BUILDPARAMS <<EOF
#!/bin/bash
#GCCBUILD=1
#MICBUILD=1
EOF
# GMP requirement
GMPlibrary=$SRC/build/gmp/xeon/lib/libgmp.a
if [ -f $GMPlibrary ];
then
echo "$GMPlibrary found"
else
echo "Building $GMPlibrary"
cd $SRC && sh $SRC/build.gmp.sh
fi
cd $SRC && sh $SRC/build.sh
else
echo "XEONTOOLS disabled"
fi
