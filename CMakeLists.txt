cmake_minimum_required(VERSION 3.5...3.31)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

# Name the project
project(ncvis 
  VERSION 0.1
  LANGUAGES CXX
)

# Set some standard settings
set (CMAKE_CXX_STANDARD 11 CACHE STRING "The C++ standard to use")
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_GNU_FLAGS "-fpermissive")

# --as-needed is a GNU ld option; Apple ld does not accept it.
if (NOT APPLE)
  set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
endif()

# Find wxWidgets libraries
find_package(wxWidgets REQUIRED gl core base OPTIONAL_COMPONENTS net)
include(${wxWidgets_USE_FILE})

# Configure with NetCDF_ROOT=$PATH_TO_NETCDF_INSTALL
find_package(NetCDF REQUIRED)

find_program(NCCONFIG_EXE NAMES nc-config PATHS ${NETCDF_DIR}/bin ${NetCDF_ROOT}/bin)

# Get all necessary config flags
execute_process(COMMAND "${wxWidgets_CONFIG_EXECUTABLE}" "--cxxflags" OUTPUT_VARIABLE WX_COMPILE_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${wxWidgets_CONFIG_EXECUTABLE}" "--prefix" OUTPUT_VARIABLE WXCONFIG_RPATH OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "${NCCONFIG_EXE}" "--cflags" OUTPUT_VARIABLE NC_COMPILE_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${NCCONFIG_EXE}" "--includedir" OUTPUT_VARIABLE NC_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_GNU_FLAGS} ${WX_COMPILE_FLAGS} ${NC_COMPILE_FLAGS} -I${NC_INCLUDE_DIR}")

# Linking is handled by target_link_libraries() in src/CMakeLists.txt using the
# find_package() results, and the rpath by BUILD_RPATH/INSTALL_RPATH on the
# target. Nothing here should parse wx-config/nc-config link output: doing so
# broke macOS builds on "-framework X" pairs, and breaks on any nc-config that
# emits CMake target names (e.g. "-lHDF5::HDF5") in its --libs --static output.

add_subdirectory("src")

