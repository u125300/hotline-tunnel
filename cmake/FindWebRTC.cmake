# The following variables will be defined:
#
#  WEBRTC_FOUND
#  WEBRTC_INCLUDE_DIR
#  WEBRTC_LIBRARIES
#

# ============================================================================
# WebRTC root and default library directory
# ============================================================================

set(WEBRTC_ROOT_DIR
  ${PROJECT_SOURCE_DIR}/../deps/webrtc/src
  CACHE PATH
  "WebRTC root directory."
  )

set(WEBRTC_LIBRARY_DIR
  ${WEBRTC_ROOT_DIR}/out/Release
  CACHE PATH
  "WebRTC release library directory that contaions webrtcm.lib or libwebrtcm.a"
  )

set(WEBRTC_DEBUG_LIBRARY_DIR
  ${WEBRTC_ROOT_DIR}/out/Debug
  CACHE PATH
  "WebRTC debug library directory that contaions webrtcm.lib or libwebrtcm.a"
  )


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
# Find WebRTC libries
#   webrtcm.lib or libwebrtcm.a (You should manually generat it.)
#   libyuv.lib or libyuv.a
# ============================================================================

find_library(_WEBRTC_RELEASE_LIB_PATH
  NAMES webrtcm libwebrtcm
  PATHS 
    ${WEBRTC_LIBRARY_DIR}
)

find_library(_YUV_RELEASE_LIB_PATH
  NAMES yuv libyuv
  PATHS
    ${WEBRTC_LIBRARY_DIR}
  )

find_library(_WEBRTC_DEBUG_LIB_PATH
    NAMES webrtcm libwebrtcm
    PATHS
      ${WEBRTC_DEBUG_LIBRARY_DIR}
      ${WEBRTC_LIBRARY_DIR}
    )

find_library(_YUV_DEBUG_LIB_PATH
  NAMES yuv libyuv
  PATHS
    ${WEBRTC_DEBUG_LIBRARY_DIR}
    ${WEBRTC_LIBRARY_DIR}
  )

list(APPEND
  WEBRTC_LIBRARIES
  optimized ${_WEBRTC_RELEASE_LIB_PATH}
  optimized ${_YUV_RELEASE_LIB_PATH}
  debug ${_WEBRTC_DEBUG_LIB_PATH}
  debug ${_YUV_DEBUG_LIB_PATH}
  )
  
if(WIN32 AND MSVC)
  list(APPEND
    WEBRTC_LIBRARIES
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
# Validation
# ============================================================================

if (WEBRTC_INCLUDE_DIR AND _WEBRTC_RELEASE_LIB_PATH)
  set(WEBRTC_FOUND 1)
else()
  set(WEBRTC_FOUND 0)
endif()

if(NOT WEBRTC_FOUND)
  message(FATAL_ERROR "\n\n!!!!! WebRTC was not found. !!!!!\nPlease specify path manually.\n\n")
endif()
