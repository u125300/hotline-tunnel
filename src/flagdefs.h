#ifndef TALK_EXAMPLES_PEERCONNECTION_CLIENT_FLAGDEFS_H_
#define TALK_EXAMPLES_PEERCONNECTION_CLIENT_FLAGDEFS_H_
#pragma once

#include "htn_config.h"
#include "webrtc/base/flags.h"

extern const uint16 kDefaultServerPort;  // From defaults.[h|cc]

// Define flags for the peerconnect_client testing tool, in a separate
// header file so that they can be shared across the different main.cc's
// for each platform.

DEFINE_bool(help, false, "Prints this message");
DEFINE_bool(server, false, "server mode");
DEFINE_string(p, "", "password");
DEFINE_string(r, "", "Room id");
DEFINE_bool(udp, false, "UDP mode");


#endif  // TALK_EXAMPLES_PEERCONNECTION_CLIENT_FLAGDEFS_H_
