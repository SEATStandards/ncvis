#!/bin/bash

# set the C++ compiler
CXX=g++

# set the install prefix
PREFIX="$(pwd -P)/"

# get the wxwidgets build flags
WXFLAGS=`wx-config --cxxflags --libs --cppflags`

# get the netCDF build flags
NCFLAGS=`nc-config --cflags --libs`

# infer the RPATH needed for dynamic linking to wxwidgets
RPATH=`wx-config --prefix`/lib

# build the executable
cd src && $CXX -std=c++11 -fpermissive -Wl,-rpath,${RPATH} -o ${PREFIX}/ncvis ncvis.cpp kdtree.cpp wxNcVisFrame.cpp wxNcVisOptionsDialog.cpp wxNcVisExportDialog.cpp wxImagePanel.cpp GridDataSampler.cpp ColorMap.cpp netcdf.cpp ncvalues.cpp Announce.cpp TimeObj.cpp ShpFile.cpp schrift.cpp lodepng.cpp ${WXFLAGS} ${NCFLAGS}
