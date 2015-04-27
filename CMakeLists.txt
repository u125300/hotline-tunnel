# ============================================================================
#
# CMake file for Hotline tunnel
#
# ============================================================================

cmake_minimum_required(VERSION 2.8)
project(HotlineTunnel)


# ============================================================================
# The version number.
# ============================================================================

set(PACKAGE "HotlineTunnel")
set(CPACK_PACKAGE_NAME "${PACKAGE}")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION 
  "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
set(CPACK_PACKAGE_VENDOR "newruns@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PACKAGE} ${PACKAGE_VERSION}")


# ============================================================================
# Find external dependencies
# ============================================================================

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/")
find_package(WebRTC)
find_package(Libwebsockets)


# ============================================================================
# The output directory.
# ============================================================================
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

message(STATUS "CMAKE_TOOLCHAIN_FILE='${CMAKE_TOOLCHAIN_FILE}'")


# ============================================================================
# Platform or compiler specific configurations
# ============================================================================
if (MSVC)

  if (NOT MSVC12)
    message(FATAL_ERROR "\n\n!!!!! Supports only Visual Studio 2013 Community or professional. !!!!!\n")
  endif()

  # Use /MT or /MTd instead of /MD or /MDd
  foreach(flag_var
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${flag_var} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif(${flag_var} MATCHES "/MD")
  endforeach(flag_var)
endif()

if (UNIX)
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -stdlib=libstdc++")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
    set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libstdc++")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  endif()
  set(WEBRTC_POSIX_OR_WIN WEBRTC_POSIX)
elseif (WIN32)
  set(WEBRTC_POSIX_OR_WIN WEBRTC_WIN)
endif()

add_definitions(-DUNICODE -D_UNICODE)
add_definitions(${WEBRTC_DEFINES})

# ============================================================================
# Generate the config.h that includes all the compilation settings.
# ============================================================================
configure_file(
  "${PROJECT_SOURCE_DIR}/htn_config.h.cmake"
  "${PROJECT_BINARY_DIR}/htn_config.h")


# ============================================================================
# Headers and sources.
# ============================================================================
set(HEADERS
  "${PROJECT_BINARY_DIR}/htn_config.h"
  "src/conductor.h"
  "src/conductors.h"
  "src/defaults.h"
  "src/socket.h"
  "src/socket_server.h"
  "src/socket_client.h"
  "src/websocket.h"
  "src/data_channel.h"
  "src/flagdefs.h"
  "src/signalserver_connection.h"
  )

set(SOURCES
  "src/main.cc"
  "src/conductor.cc"
  "src/conductors.cc"
  "src/defaults.cc"
  "src/socket.cc"
  "src/socket_server.cc"
  "src/socket_client.cc"
  "src/websocket.cc"
  "src/data_channel.cc"
  "src/signalserver_connection.cc"
  )

# ============================================================================
# Target settings
# ============================================================================

include_directories(
  "${PROJECT_BINARY_DIR}"
  "${WEBRTC_INCLUDE_DIR}"
  "${WEBRTC_INCLUDE_DIR}/webrtc"
  "${WEBRTC_INCLUDE_DIR}/third_party"
  "${WEBRTC_INCLUDE_DIR}/third_party/webrtc"
  "${WEBRTC_INCLUDE_DIR}/third_party/wtl/include"
  "${WEBRTC_INCLUDE_DIR}/third_party/jsoncpp/overrides/include"
  "${WEBRTC_INCLUDE_DIR}/third_party/jsoncpp/source/include"
  "${LIBWEBSOCKETS_INCLUDE_DIR}"
  )

add_executable(htunnel ${HEADERS} ${SOURCES})
target_link_libraries(htunnel
  ${WEBRTC_LIBRARIES}
  ${LIBWEBSOCKETS_LIBRARIES}
  )

install (TARGETS htunnel DESTINATION bin)
