find_package(PkgConfig)
pkg_check_modules(PC_ZMQ "libzmq")
set(ZMQ_DEFINITIONS ${PC_ZMQ_CFLAGS_OTHER})

find_path(ZMQ_INCLUDE_DIR 
            NAMES zmq.h
            HINTS ${PC_ZMQ_INCLUDEDIR} ${PC_ZMQ_INCLUDE_DIRS} $ENV{ZMQ_DIR}/include
            PATHS /usr/local/include 
                  /usr/include )

find_library(ZMQ_STATIC_LIBRARY
            NAMES libzmq.a
            HINTS ${PC_ZMQ_LIBDIR} ${PC_ZMQ_LIBRARY_DIRS} $ENV{ZMQ_DIR}/lib
            PATHS /usr/local/lib
                  /usr/lib)

find_library(ZMQ_LIBRARY 
            NAMES zmq
            HINTS ${PC_ZMQ_LIBDIR} ${PC_ZMQ_LIBRARY_DIRS} $ENV{ZMQ_DIR}/lib
            PATHS /usr/local/lib
                  /usr/lib)

set(ZMQ_LIBRARIES ${ZMQ_LIBRARY} )
set(ZMQ_STATIC_LIBRARIES ${ZMQ_STATIC_LIBRARY} )
set(ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR} )

message(STATUS "ZMQ LIBRARIES: " ${ZMQ_LIBRARIES})
message(STATUS "ZMQ STATIC LIBRARIES: " ${ZMQ_STATIC_LIBRARIES})
message(STATUS "ZMQ INCLUDE DIRS: " ${ZMQ_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FFTW3F_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ZMQ  DEFAULT_MSG
                                  ZMQ_LIBRARY ZMQ_INCLUDE_DIR)

mark_as_advanced(ZMQ_INCLUDE_DIR ZMQ_STATIC_LIBRARY ZMQ_LIBRARY )
