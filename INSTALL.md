# Prerequisites

  * [wxwidgets](https://docs.wxwidgets.org/3.1.7/index.html)
  * [netCDF](https://www.unidata.ucar.edu/software/netcdf/).

The prerequisites can readily be installed using the `conda` package manager:

```
conda create -n ncvis -c conda-forge wxwidgets libnetcdf
conda activate ncvis
```

Work is presently underway to put ncvis on conda-forge, and is expected for
our second release.

## Issues with Building under GNU g++ v4.8.5

Issues have been reported with ncvis segfaulting when compiled with GNU g++
version 4.8.5. A compiled executable with GNU g++ version 10 or later does
not seem to suffer from this issue.  To ensure a modern version of the GNU
compiler is used, you may wish to add `cxx-compiler` to the `conda`
environment.

## Missing libraries

Some have reported issues with missing libraries in their build environment
during build (e.g., https://github.com/SEATStandards/ncvis/issues/14).  This
may mean additional libraries need to be added via conda:

```
conda install -c anaconda libxxf86vm-cos6-x86_64
```

Often Googling for the name of the warning message will provide instructions
on the name of the package needed to address the issue.  At this point no
general solution is known.

# Install with cmake

A cmake based build infrastructure has been included to assist with
configuring the build.  To build ncvis in this manner, in the `ncvis` folder
run commands:

```
cmake .
make all
```
# Install with build.sh

The executable can also be built by running `sh ./build.sh`.  The script
has some configurable flags, such as the install prefix.  By default the
install is created in the root directory.

# Install on Cheyenne

On NCAR's Cheyenne supercomputer, by default several modules prevent static
linking with HDF5 and other dependencies when building with the netcdf
library.  To complete the build on Cheyenne the following commands should be
run prior to compilation:

```
module load gnu/12.1.0
module unload ncarenv
module unload ncarcompilers
module unload netcdf
```
# Environment variables

Resources needed by `ncvis` for execution are normally found in the `resources`
folder, which is assumed to be in the same folder as the `ncvis` executable.
If an alternate resource folder location is needed, the environment variable
`NCVIS_RESOURCE_DIR` should be set to the resources path.
