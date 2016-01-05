#!/bin/bash
# Define a timestamp function
timestamp() {
	date +"%Y%m%d%H%M%S"
}

# Build Parameters #
SRC=/home/cado-nfs
source $SRC/build_params.sh
build_logs=/mnt/nfs/System/build_logs

if test ${GCCBUILD+defined}; then
echo "GCC BUILD"
filename=$build_logs/cado-nfs-gcc-$(timestamp).txt
else
echo "ICC BUILD"
filename=$build_logs/cado-nfs-icc-$(timestamp).txt
fi

sh -x build.install.sh \
2>&1 \
| tee -a $filename

echo "done"
