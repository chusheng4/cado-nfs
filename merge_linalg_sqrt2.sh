#!/bin/sh -
# We assume that a directory $outdir exists
#
# Typical use: ./merge_linalg_sqrt2.sh params [rels1 rels2 ... relsk]
#
# $root=$dir/name is the rootname of the number to be factored, which
#                 tells us that we expect $root.{N,poly} to exist; from
#                 which we will build $root.{nodup,purged}.
# $outdir is the directory where all "temporary" files created by merge et al.
#         together with the files needed for the squareroots. If this variable
#         is not set, it defaults to $root + some suffix, see below.
#
# If no relation files are given, use $dir/rels.*
#
echo "Args: $*"

params=$1; shift

# default parameters (see README.params)

#####
. $params

if [ "X$root" = "X" ]; then echo "I need at least a name..."; exit; fi

cado=${cado-.} # where we can find linalg/, sqrt/, etc.
export cado

linalg=$cado/linalg
sqrt=$cado/sqrt/naive

keep_purge=${keep_purge-0}

nkermax=${nkermax-30}
nchar=${nchar-50}

prune=${prune-1.0}
maxlevel=${maxlevel-6}
cwmax=${cwmax-10}
rwmax=${rwmax-100}
ratio=${ratio-1.1}
coverNmax=${coverNmax-0}

# don't be verbose by default!
verbose=${verbose-""}
##### linalg params
skip=${skip-32}
bwstrat=${bwstrat-1}
########## multithreading in bw
mt=${mt-0}

outdir=${outdir-$root.$prune"x"$maxlevel"x"$cwmax"x"$rwmax}
linalg_out=${linalg_out-$outdir}

dir=`dirname $root`
if [ $# -eq 0 ]; then allrels=`ls $dir/rels.*`; else allrels="$*"; fi

echo $allrels

poly=$root.poly
nodup=$root.nodup
purged=$root.purged #.gz
tmp=$root.tmp$$

fc155=false

if $fc155
then
  echo "WARNING: special case of c155!!!!!!!!!!!!"
else ####TMP

if [ ! -s $nodup ]
then
  newrels=1
else
  echo "File $nodup already exists, checking for new relations"
  newrels=0
  for f in $allrels
  do
    if [ $f -nt $nodup ]
    then
	echo "File $f is newer than $nodup"
	newrels=1
	break
    fi
  done
fi
if [ $newrels -ne 1 ]
then
  echo "No need to rebuild $nodup"
else
  nrels=`wc -l $allrels | tail -1 | awk '{print $1}'`
  time $linalg/duplicates -nrels $nrels -out $nodup $allrels
  if [ ! -s $nodup ]; then echo "zero file $nodup"; exit; fi
fi

if [ -s $purged -a $purged -nt $nodup ]
then
  echo "File $purged already exists and is newer than $nodup"
else
  if [ "X$nrels_dup" != "X" ]
  then
    echo "I use nrels_dup=$nrels_dup"
  else
    nrels_dup=`wc -l < $nodup` ## TODO: what if nodup is gzipped?????
  fi
  args="-poly $poly -keep $keep_purge"
  if [ "X$sos" != "X" ]; then args="$args -sos $sos"; fi
  time $linalg/purge $args -nrels $nrels_dup -out $purged $nodup > $tmp
  if [ ! -s $purged ]; then echo "zero file $purged"; exit; fi
#  excess=`head -1 $purged | awk '{nrows=$1; ncols=$2; print (nrows-ncols)}'`
  excess=`grep EXCESS $tmp | awk '{print $NF}'`
  /bin/rm -f $tmp
  echo "excess = $excess"
  if [ $excess -le 0 ]
  then 
      echo "excess <= 0, sorry"
      /bin/rm -f $purged
      exit
  fi
fi

fi #######fc155

echo "Performing merges"

if [ -d $outdir ]
then
    echo "Directory $outdir already exists"
else
    echo "Creating directory $outdir"
    mkdir $outdir
fi

mergehis=$outdir/merge.his
keep=`expr 128 '+' $skip`
##### warning: first two arguments must be -out <outfile>
argsa="-out $mergehis -maxlevel $maxlevel -cwmax $cwmax -rwmax $rwmax"
argsa="$argsa $verbose -prune $prune -mat $purged -keep $keep"
argsa="$argsa -forbw $bwstrat -ratio $ratio -coverNmax $coverNmax"
#time $linalg/merge $argsa # 2> $outdir.merge.err
$cado/merge.sh $argsa
echo "SIZE(merge.his): `ls -s $mergehis`"

echo "Replaying merges"

bwcostmin=`tail $mergehis | grep "BWCOSTMIN:" | awk '{print $NF}'`
argsr="-purged $purged -his $mergehis -out $outdir/small -index $outdir/index"
if [ "X$bwcostmin" != "X" ]
then
    argsr="$argsr -costmin $bwcostmin"
fi
time $linalg/replay $argsr # 2> $outdir.replay.err

if [ ! -s $outdir/index ]
then
    echo "Index file $outdir/index does not exist, stopping"
    exit
fi
echo "SIZE(index): `ls -s $outdir/index`"

if [ ! -s $outdir/small ]
then
    echo "Small matrix $outdir/small does not exist, stopping"
    exit
fi

echo "Performing the linear algebra phase"

$cado/linalg.sh $outdir/small $skip $linalg_out $mt

if [ ! -s $linalg_out/ker_raw ]; then echo "Zerodim kernel, stopping"; exit; fi

nker=`wc -l < $linalg_out/ker_raw`
if [ $nker -lt $nkermax ]; then nkermax=$nker; fi

echo "Adding characters"

args0="-purged $purged -ker $linalg_out/ker_raw"
args0="$args0 -poly $poly -index $outdir/index"
args0="$args0 -rel $nodup -small $outdir/small"
args0="$args0 -nker $nker -nchar $nchar -skip $skip"
time $linalg/characters $args0 > $linalg_out/ker

ndepmax=`wc -l $linalg_out/ker | awk '{print $1}'`
if [ $ndepmax -ge 30 ]; then ndepmax=30; fi

echo "Preparing $ndepmax squareroots"

args1="$nodup $purged $outdir/index $linalg_out/ker $poly"
time $linalg/allsqrt $args1 0 $ndepmax ar $linalg_out/dep

echo "Entering the last phase"

$cado/newsqrtonly.sh $root $linalg_out 0 $ndepmax
## If this fails, use the backup version, based on magma:
##   ./sqrtonly.sh $root $outdir 0 $ndepmax
