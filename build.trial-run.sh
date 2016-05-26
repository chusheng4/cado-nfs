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


wORK=/home/bigspace/myfacto/c155
mkdir -p $wORK

echo "Export factorization trial data to timestamped file: $filename"
filename=$trials/E5-2603v3-MIC5110P-$(timestamp).txt
$PYTHON3 $BUILDTREE/cado-nfs.py --verbose \
workdir=$wORK \
N=10941738641570527421809707322040357612003732945449205990913842131476349984288934784717997257891267332497625752899781833797076537244027146743531593354333897 \
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
slaves.hostnames=mic0,mic1,mic2,mic3,mic4,mic5,mic6,mic7 \
slaves.nrclients=60 \
slaves.scriptpath=$SRC/build/mic \
tasks.I=12 \
tasks.polyselect.threads=4 \
tasks.polyselect.polyselect1.polyselect.execpath=$SRC/build/mic \
tasks.polyselect.admax=2e7 \
tasks.polyselect.adrange=3600 \
tasks.polyselect.nrkeep=3840 \
tasks.polyselect.polyselect2.polyselect_ropt.execpath=$SRC/build/mic \
tasks.polyselect.batch=4 \
tasks.sieve.sieving.maxfailed=1000 \
tasks.sieve.sieving.las.execpath=$SRC/build/mic \
tasks.sieve.sieving.threads=4 \
tasks.sieve.factorbase.makefb.execpath=$SRC/build/intel64 \
tasks.sieve.factorbase.threads=12 \
tasks.sieve.freerel.freerel.execpath=$SRC/build/intel64 \
tasks.sieve.freerel.threads=12 \
tasks.filter.execpath=$SRC/build/intel64 \
tasks.filter.threads=12 \
tasks.linalg.execpath=$SRC/build/intel64 \
tasks.linalg.threads=12 \
tasks.sqrt.execpath=$SRC/build/intel64 \
tasks.sqrt.threads=12 \
2>&1 \
| ts '[%Y%m%d%H%M%S]' \
| tee $filename
