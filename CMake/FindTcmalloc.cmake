# - Find Tcmalloc
# Find the native Tcmalloc includes and library
#
#  TCMALLOC_INCLUDE_DIR - where to find Tcmalloc.h, etc.
#  TCMALLOC_LIBRARIES   - List of libraries when using Tcmalloc.
#  TCMALLOC_FOUND       - True if Tcmalloc found.

find_path(TCMALLOC_INCLUDE_DIR google/tcmalloc.h 
    NO_DEFAULT_PATH
    PATHS
        /usr/include
        /usr/local/include
        /opt/local/include
)

set(TCMALLOC_NAMES tcmalloc_minimal tcmalloc)

find_library(TCMALLOC_LIBRARY NO_DEFAULT_PATH
  NAMES ${TCMALLOC_NAMES}
  PATHS 
    /lib
    /usr/lib
    /usr/local/lib
    /opt/local/lib
)

if(TCMALLOC_INCLUDE_DIR AND TCMALLOC_LIBRARY)
  set(TCMALLOC_FOUND TRUE)
else()
  set(TCMALLOC_FOUND FALSE)
endif()

if(TCMALLOC_FOUND)
    message(STATUS "Found Tcmalloc: ${TCMALLOC_LIBRARY}")
else()
    message(STATUS "Could not find tcmalloc")
endif()

mark_as_advanced(
  TCMALLOC_LIBRARY
  TCMALLOC_INCLUDE_DIR
)
