# Build Parameters #
source /home/cado-nfs/build_params.sh

if test ${GCCBUILD+defined}; then
echo "GCC BUILD"
build_tree="${up_path}/build/gcc"
PREFIX="${up_path}/build/gcc/"
GMP_INCDIR="${up_path}/build/gmp/gcc/include"
GMP_LIBDIR="${up_path}/build/gmp/gcc/lib"
else
#common to MIC and XEON
CC=mpiicc
CXX=mpiicpc
NO_PYTHON_CHECK="Python3_Requirement_Ignored"
if test ${MICBUILD+defined}; then
echo "MIC BUILD"
CFLAGS="-std=c99 -mmic -g -O3"
CXXFLAGS="-mmic -g -O3"
build_tree="${up_path}/build/mic"
PREFIX="${up_path}/build/mic/"
GMP_INCDIR="${up_path}/build/gmp/mic/include"
GMP_LIBDIR="${up_path}/build/gmp/mic/lib"
MPI=/opt/intel/compilers_and_libraries_2016.3.210/linux/mpi/intel64/bin/
OMPI_CC=/opt/intel/compilers_and_libraries_2016.3.210/linux/mpi/mic/bin/mpiicc
OMPI_CXX=/opt/intel/compilers_and_libraries_2016.3.210/linux/mpi/mic/bin/mpiicpc
NO_SSE="DISABLE_INTRINSIC"
NO_INLINE_ASSEMBLY="DISABLE_INTRINSIC"
NO_GAS_ASSEMBLY="DISABLE_INTRINSIC"
GF2X_CONFIGURE_EXTRA_FLAGS="--host=x86_64"
else
echo "XEON BUILD"
build_tree="${up_path}/build/intel64"
CFLAGS="-std=c99 -g -O3"
CXXFLAGS="-g -O3"
build_tree="${up_path}/build/intel64"
PREFIX="${up_path}/build/intel64/"
GMP_INCDIR="${up_path}/build/gmp/xeon/include"
GMP_LIBDIR="${up_path}/build/gmp/xeon/lib"
MPI=/opt/intel/compilers_and_libraries_2016.3.210/linux/mpi/intel64/bin/
OMPI_CC=/opt/intel/compilers_and_libraries_2016.3.210/linux/mpi/intel64/bin/mpiicc
OMPI_CXX=/opt/intel/compilers_and_libraries_2016.3.210/linux/mpi/intel64/bin/mpiicpc
fi
fi
