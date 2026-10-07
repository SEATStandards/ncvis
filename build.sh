#!/bin/sh
#
# INSTALL.md documents running this as "sh ./build.sh", so keep it POSIX:
# no arrays, no [[ ]], no "local".

set -eu

# set the C++ compiler (respect CXX from the environment if already set)
CXX="${CXX:-g++}"

# Set the install prefix. Resolve it to an absolute path before the cd below,
# so a relative PREFIX stays relative to where the script was invoked.
PREFIX="${PREFIX:-$(pwd -P)}"
mkdir -p "${PREFIX}"
PREFIX="$(cd "${PREFIX}" && pwd -P)"

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
# Unquoted $SOURCES below is deliberate: the word splitting is what passes the
# files as separate arguments.
SOURCES=
for f in *.cpp; do
	if [ ! -e "$f" ]; then
		echo "No .cpp files found in $(pwd)" >&2
		exit 1
	fi
	SOURCES="${SOURCES} ${f}"
done

# build the executable
# shellcheck disable=SC2086
$CXX -std=c++11 -fpermissive -Wl,-rpath,${RPATH} -o ${PREFIX}/ncvis ${SOURCES} ${WXFLAGS} ${NCFLAGS}
