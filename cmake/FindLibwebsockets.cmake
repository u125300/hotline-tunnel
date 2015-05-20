# The following variables will be defined:
#
#  LIBWEBSOCKETS_FOUND
#  LIBWEBSOCKETS_INCLUDE_DIR
#  LIBWEBSOCKETS_LIBRARIES
#

# ============================================================================
# Libwebsockets root and default library directory
# ============================================================================

set(DEPENDENCIES_ROOT_DIR
  ${PROJECT_SOURCE_DIR}/../deps
  CACHE PATH
  "deps directory."
  )

set(LIBWEBSOCKETS_ROOT_DIR
  ${DEPENDENCIES_ROOT_DIR}/libwebsockets
  CACHE PATH
  "Libwebsockets root directory."
  )

set(LIBWEBSOCKETS_LIBRARY_DIR
  ${DEPENDENCIES_ROOT_DIR}/out/Release
  CACHE PATH
  "Libwebsockets release library directory that contaions websockets_static.lib or libwebsockets.a"
  )

set(LIBWEBSOCKETS_DEBUG_LIBRARY_DIR
  ${DEPENDENCIES_ROOT_DIR}/out/Debug
  CACHE PATH
  "Libwebsockets debug library directory that contaions websockets_static.lib or libwebsockets.a"
  )

# ============================================================================
# Find Libwebsockets and dependent header directory
# ============================================================================

find_path(LIBWEBSOCKETS_INCLUDE_DIR
  NAMES
  	libwebsockets.h
  PATHS
  	${LIBWEBSOCKETS_ROOT_DIR}/lib
  )


# ============================================================================
# Find Libwebsockets libries
#   websockets_static.lib or libwebsockets.a (You should manually generat it.)
# ============================================================================

find_library(_LIBWEBSOCKETS_RELEASE_LIB_PATH
  NAMES websockets_static websockets
  PATHS 
    ${LIBWEBSOCKETS_LIBRARY_DIR}
)

find_library(_LIBWEBSOCKETS_DEBUG_LIB_PATH
    NAMES websockets_static websockets
    PATHS
      ${LIBWEBSOCKETS_DEBUG_LIBRARY_DIR}
      ${LIBWEBSOCKETS_LIBRARY_DIR}
    )

find_library(_ZLIB_RELEASE_LIB_PATH
  NAMES zlibstatic z
  PATHS 
    ${LIBWEBSOCKETS_LIBRARY_DIR}
)

find_library(_ZLIB_DEBUG_LIB_PATH
    NAMES zlibstaticd zlibd zlibstatic z
    PATHS
      ${LIBWEBSOCKETS_DEBUG_LIBRARY_DIR}
      ${LIBWEBSOCKETS_LIBRARY_DIR}
    )

list(APPEND
  LIBWEBSOCKETS_LIBRARIES
  optimized ${_LIBWEBSOCKETS_RELEASE_LIB_PATH}
  optimized ${_ZLIB_RELEASE_LIB_PATH}
  debug ${_LIBWEBSOCKETS_DEBUG_LIB_PATH}
  debug ${_ZLIB_DEBUG_LIB_PATH}
  )
  

# ============================================================================
# Validation
# ============================================================================

if (LIBWEBSOCKETS_INCLUDE_DIR AND _LIBWEBSOCKETS_RELEASE_LIB_PATH)
  set(LIBWEBSOCKETS_FOUND 1)
else()
  set(LIBWEBSOCKETS_FOUND 0)
endif()

if(NOT LIBWEBSOCKETS_FOUND)
  message(FATAL_ERROR "\n\n!!!!! Libwebsockets was not found. !!!!!\nPlease specify path manually.\n\n")
endif()
