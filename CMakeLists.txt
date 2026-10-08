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

# Find wxWidgets libraries
find_package(wxWidgets REQUIRED gl core base OPTIONAL_COMPONENTS net)
include(${wxWidgets_USE_FILE})

# Prefer the netCDF-provided package config, which defines the netCDF::netcdf
# imported target and carries its own include paths. Fall back to the bundled
# FindNetCDF.cmake module for installations that ship no config file.
# Configure with NetCDF_ROOT=$PATH_TO_NETCDF_INSTALL
find_package(netCDF CONFIG QUIET)
if (netCDF_FOUND AND TARGET netCDF::netcdf)
  set(NCVIS_NETCDF_TARGET netCDF::netcdf)
else()
  find_package(NetCDF REQUIRED)
  # FindNetCDF.cmake reports plain paths, so wrap them in an imported target to
  # give the rest of the build a single uniform name to link against.
  add_library(ncvis::netcdf INTERFACE IMPORTED)
  set_target_properties(ncvis::netcdf PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${NETCDF_INCLUDES}"
    INTERFACE_LINK_LIBRARIES "${NETCDF_LIBRARIES}"
  )
  set(NCVIS_NETCDF_TARGET ncvis::netcdf)
endif()

# wxWidgets has no package config; wx-config remains the supported way to query
# it. Only compile flags and the install prefix are taken from it -- linking
# uses the find_package() result, which keeps "-framework X" pairs intact.
execute_process(COMMAND "${wxWidgets_CONFIG_EXECUTABLE}" "--cxxflags" OUTPUT_VARIABLE WX_COMPILE_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "${wxWidgets_CONFIG_EXECUTABLE}" "--prefix" OUTPUT_VARIABLE WXCONFIG_RPATH OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(WX_COMPILE_FLAGS NATIVE_COMMAND "${WX_COMPILE_FLAGS}")

add_subdirectory("src")
