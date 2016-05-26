#!/bin/bash
# Export hybrid factorization trials data to timestamped files

# Define a timestamp function
timestamp() {
	date +"%Y%m%d%H%M%S"
}

# Build Parameters #
SRC=/home/cado-nfs
trials=/mnt/nfs/System/trials

filename=$trials/E5-2603v3-MIC5110P-BWC-$(timestamp).txt
/home/cado-nfs/build/mic/linalg/bwc/bwc.pl :complete 'thr=60x4' 'mpi=1x1' 'hosts=mic0,mic1,mic2,mic3,mic4,mic5,mic6,mic7' 'm=64' 'n=64' 'nullspace=left' 'interval=1000' 'matrix=/home/bigspace/myfacto/c120.merge.sparse.bin' 'wdir=/home/bigspace/myfacto/c120.bwc' 'interleaving=0' 'shuffled_product=1' 'cpubinding=/home/cado-nfs/build/mic/share/cado-nfs-2.2.0/misc/cpubinding.conf' \
mpi_extra_args='-env LD_LIBRARY_PATH=/opt/intel/compilers_and_libraries/linux/lib/mic:/opt/intel/impi/5.1.3.210/mic/lib:/opt/intel/compilers_and_libraries_2016.3.210/linux/compiler/lib/intel64_lin_mic/' \
2>&1 \
| ts '[%Y%m%d%H%M%S]' \
| tee $filename
