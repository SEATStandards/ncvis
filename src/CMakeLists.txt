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
        LINK_FLAGS "${NCVIS_LINKER_FLAGS}"
)

# Install target
INSTALL(TARGETS ncvis
  RUNTIME DESTINATION bin
)
