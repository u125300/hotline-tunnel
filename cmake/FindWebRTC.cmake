# The following variables will be defined:
#
#  WEBRTC_FOUND
#  WEBRTC_INCLUDE_DIR
#  WEBRTC_LIBRARIES
#

# ============================================================================
# WebRTC root directories
# ============================================================================

if (NOT WEBRTC_ROOT_DIR)
  set(WEBRTC_ROOT_DIR
    ${PROJECT_SOURCE_DIR}/../deps/webrtc/src
    CACHE PATH
    "WebRTC root directory."
    )
endif()

if (NOT WEBRTC_LIBRARY_DIR)
  set(WEBRTC_LIBRARY_DIR
    ${WEBRTC_ROOT_DIR}/out/lib
    CACHE PATH
    "WebRTC library directory that contaions webrtcm.lib or libwebrtcm.a"
    )
endif()


# ============================================================================
# Find WebRTC libries
#   webrtcm.lib or libwebrtcm.a (You should manually generat it.)
#   libyuv.lib or libyuv.a
# ============================================================================

find_library(_WEBRTC_LIB_PATH
  NAMES webrtcm libwebrtcm
  PATHS ${WEBRTC_LIBRARY_DIR}
  )

find_library(_YUV_LIB_PATH
  NAMES yuv libyuv
  PATHS ${WEBRTC_LIBRARY_DIR}
  )

set( WEBRTC_LIBRARIES
  "${_WEBRTC_LIB_PATH}"
  "${_YUV_LIB_PATH}"
  )

if(WIN32 AND MSVC)
  list(APPEND WEBRTC_LIBRARIES
    Secur32.lib Winmm.lib msdmo.lib dmoguids.lib wmcodecdspuuid.lib
    wininet.lib dnsapi.lib version.lib ws2_32.lib
    )
endif()

if(UNIX)
  find_package (Threads REQUIRED)
  if (APPLE)
    find_library(FOUNDATION_LIBRARY Foundation)
    find_library(CORE_FOUNDATION_LIBRARY CoreFoundation)
    find_library(CORE_SERVICES_LIBRARY CoreServices)
  endif()
  list(APPEND WEBRTC_LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    ${FOUNDATION_LIBRARY}
    ${CORE_FOUNDATION_LIBRARY}
    ${CORE_SERVICES_LIBRARY}
    )
endif()

# ============================================================================
# Find WebRTC header directory
# ============================================================================

find_path(WEBRTC_INCLUDE_DIR
  NAMES
  	webrtc/config.h
  PATHS
  	${WEBRTC_ROOT_DIR}
  )


# ============================================================================
# Validation
# ============================================================================

if (WEBRTC_INCLUDE_DIR AND WEBRTC_LIBRARIES)
  set(WEBRTC_FOUND 1)
else()
  set(WEBRTC_FOUND 0)
endif()

if(NOT WEBRTC_FOUND)
  message(FATAL_ERROR "\n\n!!!!! WebRTC was not found. !!!!!\nPlease specify path manually.\n\n")
endif()
