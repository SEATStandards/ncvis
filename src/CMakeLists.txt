# Every .cpp in this directory is part of the program, so discover them rather
# than maintaining a list by hand. build.sh globs the same way, which keeps the
# two build paths from drifting apart when a source file is added.
file(GLOB NCVIS_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(SORT NCVIS_SOURCE_FILES)

if (NOT NCVIS_SOURCE_FILES)
  message(FATAL_ERROR "No .cpp files found in ${CMAKE_CURRENT_SOURCE_DIR}")
endif()

# Make an executable target and pass all necessary source files needed to build
add_executable(ncvis ${NCVIS_SOURCE_FILES})

# All users of this library will need at least C++11
set_target_properties(ncvis
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
        BUILD_RPATH "${WXCONFIG_RPATH}/lib"
        INSTALL_RPATH "${WXCONFIG_RPATH}/lib"
)

target_include_directories(ncvis PRIVATE ${wxWidgets_INCLUDE_DIRS})

# wx-config --cxxflags carries the wxWidgets feature defines (wxUSE_*, the
# toolkit selection) that the headers require.
target_compile_options(ncvis PRIVATE ${WX_COMPILE_FLAGS})

# The vendored legacy netCDF C++ API relies on conversions that GCC rejects by
# default. -fpermissive is a GNU option; other compilers either ignore it or
# error on it, so limit it to the compilers that need it.
target_compile_options(ncvis PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:-fpermissive>
)

# Link libraries after object files (required for GNU ld).
# Use the results of find_package rather than parsing wx-config/nc-config
# output: those are already correct lists and preserve multi-token items such
# as "-framework Cocoa", which token-by-token filtering breaks on macOS.
target_link_libraries(ncvis PRIVATE ${wxWidgets_LIBRARIES} ${NCVIS_NETCDF_TARGET})

# Install target
INSTALL(TARGETS ncvis
  RUNTIME DESTINATION bin
)

# ncvis cannot start without its colormaps, fonts and shapefiles, so install
# them alongside the binary. ncvis.cpp looks here relative to the executable.
INSTALL(DIRECTORY "${ncvis_SOURCE_DIR}/resources"
  DESTINATION share/ncvis
)
