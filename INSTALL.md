# Prerequisites

  * [wxwidgets](https://docs.wxwidgets.org/3.1.7/index.html)
  * [netCDF](https://www.unidata.ucar.edu/software/netcdf/).

The prerequisites can readily be installed using the `conda` package manager:

```bash
conda create -n ncvis -c conda-forge wxwidgets libnetcdf
conda activate ncvis
```

# Install

The executable can be built simply by running `sh ./src/build.sh`.  The script
has some configurable flags, such as the install prefix.  By default the
install is created in `/.../bin/` of the source tree.
