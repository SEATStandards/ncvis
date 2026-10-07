#!/bin/bash

set -eu

# set the C++ compiler (respect CXX from the environment if already set)
CXX="${CXX:-g++}"

# set the install prefix
PREFIX="${PREFIX:-$(pwd -P)}"

# get the wxwidgets build flags
WXFLAGS=`wx-config --cxxflags --libs --cppflags`

# get the netCDF build flags
NCFLAGS=`nc-config --cflags --libs`

# infer the RPATH needed for dynamic linking to wxwidgets
RPATH=`wx-config --prefix`/lib

cd src

# Every .cpp here is part of the program, so discover them rather than keeping
# a list by hand. src/CMakeLists.txt globs the same way; listing sources in two
# places is how a new file ends up building under one path and not the other.
SOURCES=(*.cpp)
if [ ! -e "${SOURCES[0]}" ]; then
	echo "No .cpp files found in $(pwd)" >&2
	exit 1
fi

# build the executable
$CXX -std=c++11 -fpermissive -Wl,-rpath,${RPATH} -o ${PREFIX}/ncvis "${SOURCES[@]}" ${WXFLAGS} ${NCFLAGS}
