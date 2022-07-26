cmake_minimum_required(VERSION 3.1)

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
set (CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
set (CMAKE_GNU_FLAGS "-fpermissive")

# Find wxWidgets libraries
find_package(wxWidgets REQUIRED gl core base OPTIONAL_COMPONENTS net)
include(${wxWidgets_USE_FILE})

# Configure with NetCDF_ROOT=$PATH_TO_NETCDF_INSTALL
find_package(NetCDF REQUIRED)

# Get all necessary config flags
execute_process(COMMAND "nc-config" "--cflags" OUTPUT_VARIABLE NC_COMPILE_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "wx-config" "--cxxflags" OUTPUT_VARIABLE WX_COMPILE_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "nc-config" "--libs" OUTPUT_VARIABLE NC_LINK_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "wx-config" "--libs" OUTPUT_VARIABLE WX_LINK_FLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_GNU_FLAGS} ${NC_COMPILE_FLAGS} ${WX_COMPILE_FLAGS}")

add_subdirectory("src")

