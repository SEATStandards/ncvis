cmake_minimum_required(VERSION 3.2)

# Name the project
project(ncvis)

find_package(wxWidgets REQUIRED gl core base OPTIONAL_COMPONENTS net)
include(${wxWidgets_USE_FILE})
# and for each of your dependent executable/library targets:
# target_link_libraries(ncvis ${wxWidgets_LIBRARIES})

# Configure with NetCDF_ROOT=$PATH_TO_NETCDF_INSTALL
find_package(NetCDF REQUIRED)

execute_process(`nc-config --cflags` OUTPUT_VARIABLE NC_COMPILE_FLAGS)
execute_process("wx-config --cxxflags --cppflags" OUTPUT_VARIABLE WX_COMPILE_FLAGS)
execute_process(`nc-config --libs` OUTPUT_VARIABLE NC_LINK_FLAGS)
execute_process("wx-config --libs" OUTPUT_VARIABLE WX_LINK_FLAGS)

SET(NCVIS_SRC_FILES src/ncvis.cpp 
                    src/kdtree.cpp 
                    src/wxNcVisFrame.cpp 
                    src/wxNcVisOptsDialog.cpp 
                    src/wxImagePanel.cpp 
                    src/GridDataSampler.cpp 
                    src/ColorMap.cpp 
                    src/netcdf.cpp 
                    src/ncvalues.cpp 
                    src/Announce.cpp 
                    src/TimeObj.cpp 
                    src/ShpFile.cpp 
                    src/schrift.cpp
                )

include_directories(${NetCDF_INCLUDE_DIRS} ${wxWidgets_INCLUDE_DIRS})
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} ${NC_COMPILE_FLAGS} ${WX_COMPILE_FLAGS})

add_executable(ncvis ${NCVIS_SRC_FILES})
target_link_options(ncvis ${WX_LINK_FLAGS} ${NC_LINK_FLAGS}) # ${wxWidgets_LIBRARIES}

