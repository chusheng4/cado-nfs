Quick install:
==============

in most cases the following should work to factor the number 903...693
using all cores on the local machine

1) make
2) ./cado-nfs.py 90377629292003121684002147101760858109247336549001090677693

More details follow.

Important note: For a larger factorization (distributed on several
machines), the command line to be used is a priori more involved. Please
refer to scripts/cadofactor/README. Documented example parameter files
are in parameters/factor/params.c90 and scripts/cadofactor/parameters.

Supported platforms:
====================

 - The primary development platform is x86_64 linux with gcc 4.4 or
   later, the most common processor being Intel core2-like or more recent.
   
Other architectures are checked regularly, and should work. Please refer to
http://ci.inria.fr/cado for the status of the currently tested platforms
for the development branch.

Release 2.2 has undergone regular testing for the following
hardware/software configurations.

    - i386, CentOS 6.3
    - x86_64, Fedora 17, Ubuntu 12.04, Debian 8, OpenBSD 5.3
    - x86_64, MacOS X 10.8
    - arm32, Raspbian 7
    - arm64, Ubuntu 14.04

All checks above are done with gcc-4.4 or later.

Anything else perhaps works, perhaps does not work.

 - x86_64 with icc 14 and 16 did work once, but are not checked regularly.

 - Mac OS X Mavericks have been successfully tested with both
   Apple's gcc and clang. (see local.sh.macosx.x86_64) 

 - Max OS X Leopard and Snow Leopard have been tested to work at
   least once, provided some additional tools and libraries (see below) have
   been installed, notably for Leopard. ABI selection is sometimes tricky (see
   local.sh.macosx.x86_32)

 - Windows is only partly supported (see a longer note at the end of this
   file).

Required software tools:
========================

*) GMP (http://gmplib.org): usually installed in most Linux distributions
   (on some Linux distributions you need to install libgmp*-dev
    including gmp.h, which is not installed by default).
   Note: make sure to configure GMP with --enable-shared so that a shared
   library is installed (libgmp.so under Linux) otherwise CADO-NFS might not
   compile. GMP 5 (or newer) is required. On some platforms, --disable-alloca
   is required to get thread-safe operations.
*) The GCC compiler works ; some others might, but it is untested. C and
   C++ are required, with C99 support.
*) GNU make and CMake >= 2.8 for building (CMake is installed on the fly if
   missing (requires an Internet connection))
*) Support for posix threads.
*) The main cado-nfs.py script uses a lot of unix tools: Python3, ssh,
   rsync, gzip, and libsqlite3 to mention but a few. For more information
   about the supported Python versions and required modules, see
   README.Python.

Optionally:
*) Support for MPI (see local.sh.example and linalg/bwc/README)
*) Support for hwloc (see parameters/misc/cpubinding.conf)
*) Support for curl library, for a very specific use within an I/O bound
subprogram in linear algebra.


Configure
=========

Normally you have nothing to do to configure cado-nfs.

However if your site needs tweaks, set such tweaks using environment variables,
or via the shell script local.sh ; you may start with

cp local.sh.example local.sh

Edit according to your local settings and your taste: local.sh.example
gives documentation on the recognized environment variables and their effect.

Note that tweaks in local.sh are global (relevant to all sub-directories
of the cado-nfs tree, not to a particular directory).

As a rule of thumb, whenever you happen to modify the local.sh script, it
is advised to trigger re-configuration of the build system, by the
special command ``make cmake''. Another way to achieve the same goal is
to remove the build tree, which is below build/ (normally named as the
hostname of the current machine): ``make tidy'' should do it.

Optional (alternative): configure using cmake directly
======================================================

cado-nfs includes a top-level Makefile which builds the binary objects in
a special build directory which is below build/ (normally named as the
hostname of the current machine). This way, parallel builds for different
machines may coexist in one shared directory. This is sort of a magic
``out-of-source'' build.

Another way to do ``out-of-source'' build is to create a separate build
directory and build from there, by calling cmake directly for the
configuration. This proceeds as follows:

mkdir /some/build/directory
cd /some/build/directory
cmake /path/to/cado-nfs/source/tree

Note however that in this case, the local.sh file found in the source
tree is not read (but you may elect to do so before calling cmake).

Compile:
========

make

Install:
========

The relevance of the ``make install'' step depends on the platform.
Cado-nfs binaries link to shared libraries, and some do so dynamically.
For this to work, we rely on some control logic by cmake and cooperation
with the operating system's dynamic linker.

(i) if "make" is directly called from the source directory $SRCDIR,
    "make install" installs all programs and binaries in $SRCDIR/installed.

(ii) otherwise programs and binaries will be installed in
     /usr/local/share/cado-nfs-x.y.z, and this default installation prefix
     can be changed by one of the following commands:

     cmake .../cado-nfs -DCMAKE_INSTALL_PREFIX=/tmp/install

     export PREFIX=/tmp/install ; cmake .../cado-nfs

Calling one of cado-nfs scripts (e.g., cado-nfs.py) can be done in
several ways. Here we assume that $SRCDIR is the source directory, that
$BUILDDIR is the build tree for the local machine (typically
$SRCDIR/`hostname`), and that $PREFIX is the installation prefix (see above).
We refer to these different ways, and later
discuss how they work on different systems (which is mostly impacted by
the shared library mechanism).

(1) $SRCDIR/cado-nfs.py
    This deduces $BUILDDIR from the machine hostname, and amounts to
    calling binaries from there. Parameter files are obtained from
    $SRCDIR/parameters/

(2) $PREFIX/bin/cado-nfs.py
    This calls binaries from $PREFIX/bin, and loads parameter files from
    $PREFIX/share/cado-nfs-x.y.z/

(3) $BUILDDIR/cado-nfs.py
    This is not supported. Might work, might not. You've been warned.

Linux, BSD: (1) and (2) should work ok.
MacOS X:
    For any invocation to work, the LD_LIBRARY_PATH or DYLD_LIBRARY_PATH
    variable must be set up properly. The easiest method is to do make
    install, and include in these environment variables the directory
    $PREFIX/lib/cado-nfs-x.y.z.

Run a factorization on current machine:
=======================================

./cado-nfs.py 90377629292003121684002147101760858109247336549001090677693 -t 2

where the option '-t 2' tells how many cores (via threads) to use on the
current machine (for polynomial selection, sieving, linear algebra, among
others).  It is possible to set '-t all' (which, in fact, is the default)
to use all threads on the current machine.

CADO-NFS is optimized only for numbers above 85 digits, and no support will
be provided for numbers of less than 60 digits. Note that it is a good idea
to remove small prime factors using special-purpose algorithms such as trial
division, P-1, P+1, or ECM, and use CADO-NFS only for the remaining composite
factor.

Parts of the Number Field Sieve computation are massively distributed. In
this mode, client scripts (namely, cado-nfs-client.py) are run on many
nodes, connect to a central server, and run programs according to which
computations need to be done.  The programs (for the polynomial selection
and sieving steps) can run multithreaded, but it is better to have them
run with a capped number of threads (say, 2), and run several clients per
node. By default, programs in this step are limited to 2 threads. When
running the computation on the local machine, the number of clients is
set so that the number of cores specified by -t are kept busy.

Run a factorization on several machines:
=======================================

./cado-nfs.py 353493749731236273014678071260920590602836471854359705356610427214806564110716801866803409 slaves.hostnames=hostname1,hostname2,hostname3 --slaves 4 --client-threads 2

This starts 4 clients each on hosts hostname1, hostname2, and hostname3,
where each client uses two cpus (threads). For hostnames that are not
localhost, ssh is used to connect to the host and start a client there.
To configure ssh, see the next section. For tasks which use the local
machine only (not massively distributed tasks), the number of threads
used is the one given by -t (which defaults to all threads on the local
machine).

For a larger factorization (distributed on several machines, possibly
with different parameters for different machines), please use the
--server mode (see scripts/cadofactor/README and
scripts/cadofactor/parameters).

Check that your SSH configuration is correct:
=============================================

The master script (unless in --server mode) uses SSH to connect to
available computing resources.  In order to avoid the script asking your
password or passphrase, you must have public-key authentication and an
agent.

The SSH keys are usually installed in the files ~/.ssh/id_rsa and
~/.ssh/id_rsa.pub; if you don't have them yet, you can create them with the
ssh-keygen command. See the man page ssh-keygen(1) for details. The private
key should be protected with a passphrase, which you can enter when you
create the keys. Normally ssh will ask for the key's passphrase when you log
on to a machine, but this can be avoided by using ssh-agent, see the man
page ssh-agent(1), and providing the passphrase to the agent with ssh-add.
Public-key authenticaton together with an ssh-agent will allow cadofactor
to use ssh to run commands on slave machines automatically.

Most of the recent Linux distributions will run an ssh-agent for you. But
if this is not the case with your distribution, or if you are running
cado-nfs.py inside a ``screen'' in order to logout from your desktop, you
will need to run the ssh-agent by hand. As a short recipe, you can type:
   eval `ssh-agent`
   ssh-add

You should also copy your public key, i.e., the contents of the file 
~/.ssh/id_rsa.pub, into $HOME/.ssh/authorized_keys on the slave machines, to
allow logging in with public-key authentication.

Also, since localhost has an IP and key that varies, you should have
those 3 lines in your $HOME/.ssh/config:

Host    localhost
        StrictHostKeyChecking no
        UserKnownHostsFile /dev/null

If everything is correctly configured, when you type

ssh localhost

you should end up with a new shell on your machine, without having to
type any password/passphrase.


Restarting an interrupted factorization:
========================================

If you have started a factorization with the cado-nfs.py script, and it was
interrupted (for example because of a power failure) you can restart in
any of these two ways:
 - with the same cado-nfs.py command line if a work directory was
   explicitly provided on the command line:

   $ cado-nfs.py ... workdir=/path/to/workdir

 - with a single argument as in:

   $ cado-nfs.py     [[work directory]]/XXX.parameters_snapshot.YYY

   where [[work directory]] is the directory which has been chosen
   automatically, XXX is the "name" of the running factorisation, and YYY
   is the largest possible integer value for which such a file exists.

Factoring with SNFS:
====================

It is possible to take advantage of the special form of an integer and
use the Special Number Field Sieve. See parameters/factor/parameters.F9
for that:

$ cado-nfs.py parameters/factor/parameters.F9 slaves.hostnames=localhost

Note in particular that you can force the special-q to be on
the rational side if this is more appropriate for your number, with
tasks.sieve.sqside=0 on the cado-nfs.py command line or in the parameter
file (assuming you have put the side 0 is the rational side).

The default square root algorithm does not work in some very rare cases
that could possibly occur with SNFS polynomials (a degree 4 polynomial
with Galois group Z/2 x Z/2 is the only reasonable example, next case is
for degree 8). The CRT approach is a workaround. See crtaglsqrt.c .

Big factorization (more than 200 digits):
=========================================

By default, to decrease memory usage, it is assumed than less than 2^32 (~ four
billion) relations or ideals are needed and that the ideals will be less than
2^32 (i.e., the lpba/lpbr parameters are less or equal to 32). In the case of
factorizations of numbers of more than 200 digits, these assumptions may not
hold. In this case, you have to set some variables in your local.sh script
(see Configure section above for more information on local.sh and section on
big factorizations in local.sh.example).

Factoring with two non-linear polynomials:
==========================================

You can find a bit of information on this topic in the development version,
in the GIT repository (see file README.nonlinear).

Importing polynomials or relations:
===================================

If you have already computed a good polynomial and/or relations, you can
tell CADO-NFS to use them, see scripts/cadofactor/README.

Using CADO-NFS under Windows:
=============================

Portability of CADO-NFS on Windows was not an initial goal of that project,
however we give here some hints that might help people wanting to use CADO-NFS
under Windows:

* if you only need the siever to run on Windows, then you only need to compile
  the "las" program on Windows.

* CygWin provides a Unix-like environment, where compilation should be easy.
  However the binary requires a cygwin.dll file. We have been told of problems
  with shared libraries, the following solves this problem:
  PATH="installed/lib/cado-nfs-x.y.z:$PATH" ./cado-nfs.py [...]

* if you want a binary without any dependency, you might try MinGW. The INSTALL
  file from GNU MPFR contains detailed instructions on how to compile MPFR
  under Windows. Those instructions should work for CADO-NFS too.
  See dev_docs/howto-MinGW.txt. Compilation on MinGW32 is tested regularly
  and should work, although performance of the 32-bit executables is poor.

* you might try to use MPIR (mpir.org) instead of GMP. MPIR is a fork of GMP,
  which claims to be more portable under Windows.

* you might succeed in compiling the cado-nfs binaries with a
  cross-compiler for Windows (which does not waive the runtime
  requirements for cado-nfs.py, notably on unix-like utilities). See
  dev_docs/README.cross in the source code repository for information on
  how to cross-compile.

Examples of basic usage:
========================

* Run a full factorization on the local machine, using all available
  cores:

./cado-nfs.py 90377629292003121684002147101760858109247336549001090677693 

* Run a full factorization on the local machine, using 8 threads for the
  server (this includes the linear algebra), and 4 jobs of 2 threads
  each for the polynomial selection and the sieving:

./cado-nfs.py 90377629292003121684002147101760858109247336549001090677693 --slaves 4 --client-threads 2 --server-threads 8

* Run a factorization in the given directory, interrupt it (with Ctrl-C,
  or whatever unexpected event), and resume the computation:

./cado-nfs.py 90377629292003121684002147101760858109247336549001090677693 workdir=/tmp/myfacto
[Ctrl-C]
./cado-nfs.py /tmp/myfacto/c60.parameters_snapshot.0 

* Run a server on machine1, and a slave on machine2, disabling ssl:

machine1$ ./cado-nfs.py --server 90377629292003121684002147101760858109247336549001090677693 server.port=4242 server.ssl=no server.whitelist=machine2
machine2$ ./cado-nfs-client.py --server=http://machine1:4242

Note: if you are on an insecure network, you'll have to activate ssl, and
then pass the appropriate sha1 certificate to the client (the server
prints the appropriate command-line to be copy-pasted on machine2).

* Run a factorization on machine1, and have it start automatically a
  slave on machine2 via SSH:

./cado-nfs.py 90377629292003121684002147101760858109247336549001090677693 
slaves.hostnames=machine1,machine2

Note that, in that case, you have to specify machine1 as well in the list
of hostnames if you want it to contribute to the polynomial selection and
the sieving.


Known problems:
===============

* when running the square root step in multi-thread mode (tasks.sqrt.threads=2
  or larger) with GMP <= 6.0, you might encounter an issue due to a "buglet"
  in GMP (https://gmplib.org/list-archives/gmp-bugs/2015-March/003607.html).
  Workaround: use tasks.sqrt.threads=1 or GMP >= 6.1.0.
* GCC 4.1.2 is known to miscompile CADO-NFS (see
  https://gforge.inria.fr/tracker/index.php?func=detail&aid=14490),
  GCC 4.2.0, 4.2.1 and 4.2.2 are also affected.
* under NetBSD 5.1 amd64, Pthreads in the linear algebra step seem not to
  work, please use -t 1 option in cado-nfs.py or tasks.linalg.threads=1x1.
* under AIX, if GMP is compiled in 64-bit mode, you should set the
  environment variable OBJECT_MODE, for example:
  export OBJECT_MODE=64

Publications related to CADO-NFS:
=================================

Square Root Algorithms for the Number Field Sieve, Emmanuel Thomé, proceedings
of WAIFI 2012, F. Özbudak and F. Rodriguez-Henriquez (Eds), LNCS 7369,
pages 208-224, 2012.

Factorisation of RSA-704 with CADO-NFS, Shi Bai, Emmanuel Thomé and Paul
Zimmermann, preprint, http://eprint.iacr.org/2012/369, 2012.

Better Polynomials for GNFS. Shi Bai, Cyril Bouvier, Alexander Kruppa and
Paul Zimmermann. Mathematics of Computation, to appear.

Root Optimization of Polynomials in the Number Field Sieve, Shi Bai, Richard
Brent and Emmanuel Thomé. Mathematics of Computation, 2015.

Non-Linear Polynomial Selection for the Number Field Sieve, Thomas Prest and
Paul Zimmermann, Journal of Symbolic Computation, special issue in the honour
of Joachim von zur Gathen, volume 47, number 4, pages 401-409, 2011.

Améliorations de la multiplication et de la factorisation d'entier, Alexander
Kruppa, PhD thesis, Université Henri Poincaré - Nancy I, 2010, available from
http://tel.archives-ouvertes.fr/tel-00477005/en/.

Contact, links:
===============

The website of the project is hosted at:
   http://cado-nfs.gforge.inria.fr/

You can get the latest development version with:
   git clone git://scm.gforge.inria.fr/cado-nfs/cado-nfs.git

There are two mailing-lists associated to Cado-nfs:
  - cado-nfs-commits: if you want to receive an email each time a
    modification to the development version is committed to the
    repository.
  - cado-nfs-discuss: for general discussions about cado-nfs.
Instructions about how to subscribe are available at
   http://gforge.inria.fr/mail/?group_id=2065

If you find a bug, if you have a problem to compile, if you want to
factor a large number and seek for advice for tuning the parameters, then
the cado-nfs-discuss list is the right place to ask.

On the https://gforge.inria.fr/projects/cado-nfs/ web page you will also find
some forums and a bug tracker. We recommend *not to use* the forum: we don't
read it. As for the bug tracker, this is an important piece of the
cado-nfs development cycle. Submitting bugs there is welcome (you need a
gforge account), although if you are unsure, it might be better to speak
up on the cado-nfs-discuss mailing list first.
