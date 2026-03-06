# Get all header files
file(GLOB NCVIS_HEADER_FILES CONFIGURE_DEPENDS "${ncvis_SOURCE_DIR}/src/*.h")

# Can also glob source files for simplicity, but just listing explicitly for now
set(NCVIS_SOURCE_FILES
  kdtree.cpp 
  wxNcVisFrame.cpp 
  wxNcVisOptionsDialog.cpp
  wxNcVisExportDialog.cpp
  wxImagePanel.cpp 
  GridDataSampler.cpp 
  ColorMap.cpp 
  netcdf.cpp 
  ncvalues.cpp 
  Announce.cpp 
  TimeObj.cpp 
  ShpFile.cpp 
  schrift.cpp
  lodepng.cpp
)

# 3rd party include directories
include_directories(${NetCDF_INCLUDE_DIRS} ${wxWidgets_INCLUDE_DIRS})

# Make an executable target and pass all necessary source files needed to build
add_executable(ncvis  ${NCVIS_SOURCE_FILES} ncvis.cpp)

# We need this directory, and users of our library will need it too
#target_include_directories(ncvis PUBLIC .)

# All users of this library will need at least C++11
set_target_properties(ncvis
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        BUILD_RPATH "${WXCONFIG_RPATH}/lib"
        INSTALL_RPATH "${WXCONFIG_RPATH}/lib"
)

# Link libraries after object files (required for GNU ld)
separate_arguments(WX_LINK_ITEMS NATIVE_COMMAND "${WX_LINK_FLAGS}")
separate_arguments(NC_LINK_ITEMS NATIVE_COMMAND "-lnetcdf ${NC_LINK_FLAGS}")
target_link_directories(ncvis PRIVATE ${NC_LIB_DIR})

# Split wxWidgets link items into libraries (-l...) and other linker options
set(WX_LIBRARIES "")
set(WX_OPTIONS "")
foreach(item IN LISTS WX_LINK_ITEMS)
    if(item MATCHES "^-l.+")
        list(APPEND WX_LIBRARIES "${item}")
    else()
        list(APPEND WX_OPTIONS "${item}")
    endif()
endforeach()

# Split NetCDF link items into libraries (-l...) and other linker options
set(NC_LIBRARIES "")
set(NC_OPTIONS "")
foreach(item IN LISTS NC_LINK_ITEMS)
    if(item MATCHES "^-l.+")
        list(APPEND NC_LIBRARIES "${item}")
    else()
        list(APPEND NC_OPTIONS "${item}")
    endif()
endforeach()

# Link true libraries
target_link_libraries(ncvis PRIVATE ${WX_LIBRARIES} ${NC_LIBRARIES})

# Apply non-library linker options
target_link_options(ncvis PRIVATE ${WX_OPTIONS} ${NC_OPTIONS})

# Install target
INSTALL(TARGETS ncvis
  RUNTIME DESTINATION bin
)
