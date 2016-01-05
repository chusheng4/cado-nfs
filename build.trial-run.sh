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
BUILDTREE=$SRC/build/intel64
PYTHON3="python3"

echo "Export RSA-120 factorization trial data to timestamped file: $filename"
digits=c120
# Clean the tmp directory
tmpdir=$SRC/trials/$digits
rm -rf $tmpdir
mkdir -p $tmpdir
chmod 755 $tmpdir
filename=$trials/E5-2603v3-MIC5110P-$(timestamp).txt
$PYTHON3 $BUILDTREE/cado-nfs.py \
N=227010481295437363334259960947493668895875336466084780038173258247009162675779735389791151574049166747880487470296548479 \
server.address=192.168.1.80 \
server.only_registered=False \
server.port=8012 \
server.ssl=no \
server.threaded=True \
server.whitelist=192.168.1.0/30 \
slaves.hostnames=mic0,mic1,mic2,mic3,mic4,mic5,mic6,mic7 \
slaves.nrclients=60 \
slaves.scriptpath=$SRC/build/mic \
tasks.polyselect.threads=4 \
tasks.polyselect.polyselect1.polyselect.execpath=$SRC/build/mic \
tasks.polyselect.admax=1728000 \
tasks.polyselect.adrange=3600 \
tasks.polyselect.nrkeep=3840 \
tasks.polyselect.polyselect2.polyselect_ropt.execpath=$SRC/build/mic \
tasks.polyselect.batch=4 \
tasks.sieve.sieving.las.execpath=$SRC/build/mic \
tasks.sieve.sieving.threads=4 \
tasks.sieve.factorbase.makefb.execpath=$SRC/build/xeon \
tasks.sieve.factorbase.threads=12 \
tasks.sieve.freerel.freerel.execpath=$SRC/build/xeon \
tasks.sieve.freerel.threads=12 \
tasks.filter.execpath=$SRC/build/xeon \
tasks.filter.threads=12 \
tasks.linalg.execpath=$SRC/build/xeon \
tasks.linalg.threads=12 \
tasks.sqrt.execpath=$SRC/build/xeon \
tasks.sqrt.threads=12 \
tasks.workdir=$tmpdir \
2>&1 \
| ts '[%Y%m%d%H%M%S]' \
| tee $filename
