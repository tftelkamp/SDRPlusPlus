find_path(VRT_INCLUDE_DIR 
            NAMES vrt/vrt_init.h
            HINTS $ENV{VRT_DIR}/include
            PATHS /usr/local/include 
                  /usr/include )

find_library(VRT_STATIC_LIBRARY
            NAMES libvrt.a
            HINTS $ENV{VRT_DIR}/lib
            PATHS /usr/local/lib
                  /usr/lib)

set(VRT_LIBRARIES ${VRT_STATIC_LIBRARY} )
set(VRT_INCLUDE_DIRS ${VRT_INCLUDE_DIR} )

message(STATUS "VRT LIBRARIES: " ${VRT_LIBRARIES})
message(STATUS "VRT INCLUDE DIRS: " ${VRT_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FFTW3F_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(VRT  DEFAULT_MSG
                                  VRT_STATIC_LIBRARY VRT_INCLUDE_DIR)

mark_as_advanced(VRT_INCLUDE_DIR VRT_STATIC_LIBRARY)
