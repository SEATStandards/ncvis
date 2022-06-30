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

# create the install directory
mkdir -p ${PREFIX}/bin

# build the executable
cd src && $CXX -std=c++11 -fpermissive ${WXFLAGS} ${NCFLAGS} -Wl,-rpath,${RPATH} -o ${PREFIX}/bin/ncvis ncvis.cpp kdtree.cpp wxNcVisFrame.cpp wxImagePanel.cpp GridDataSampler.cpp ColorMap.cpp netcdf.cpp ncvalues.cpp Announce.cpp schrift.cpp
