#!/bin/bash
# Export hybrid factorization trials data to timestamped files

# Define a timestamp function
timestamp() {
	date +"%Y%m%d%H%M%S"
}

# Build Parameters #
SRC=/home/cado-nfs
trials=/mnt/nfs/System/trials

echo "HYBRID BUILD"
BUILDTREE=$SRC/build/mic
PYTHON3="python3"

DATADIR=/home/bigspace/myfacto/dev
rm -rf $DATADIR
mkdir -p $DATADIR

echo "Export factorization trial data to timestamped file: $filename"
filename=$trials/E5-2603v3-MIC5110P-$(timestamp).txt
$PYTHON3 $BUILDTREE/cado-nfs.py \
workdir=$DATADIR \
N=1522605027922533360535618378132637429718068114961380688657908494580122963258952897654000350692006139 \
server.address=172.31.1.254 \
server.only_registered=False \
server.port=8012 \
server.ssl=no \
server.threaded=True \
server.whitelist=\
172.31.1.254,\
172.31.2.254,\
172.31.3.254,\
172.31.4.254,\
172.31.5.254,\
172.31.6.254,\
172.31.7.254,\
172.31.8.254 \
slaves.hostnames=mic0,mic1 \
slaves.nrclients=60 \
slaves.scriptpath=$SRC/build/mic \
tasks.polyselect.threads=4 \
tasks.polyselect.polyselect1.polyselect.execpath=$SRC/build/mic \
tasks.polyselect.polyselect2.polyselect_ropt.execpath=$SRC/build/mic \
tasks.polyselect.batch=4 \
tasks.sieve.sieving.las.execpath=$SRC/build/mic \
tasks.sieve.sieving.threads=4 \
tasks.sieve.factorbase.makefb.execpath=$SRC/build/intel64 \
tasks.sieve.factorbase.threads=12 \
tasks.sieve.freerel.freerel.execpath=$SRC/build/intel64 \
tasks.sieve.freerel.threads=12 \
tasks.filter.execpath=$SRC/build/intel64 \
tasks.filter.threads=12 \
tasks.linalg.execpath=$SRC/build/mic \
tasks.linalg.threads=240 \
tasks.linalg.bwc.bwc.hostfile=$SRC/bwchosts \
tasks.linalg.bwc.bwc.mpi=60x2 \
tasks.linalg.bwc.bwc.threads=4x1 \
tasks.sqrt.execpath=$SRC/build/intel64 \
tasks.sqrt.threads=12 \
2>&1 \
| ts '[%Y%m%d%H%M%S]' \
| tee $filename
