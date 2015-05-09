#ifndef HOTLINE_TUNNEL_FLAGDEFS_H_
#define HOTLINE_TUNNEL_FLAGDEFS_H_
#pragma once

#include "htn_config.h"
#include "webrtc/base/flags.h"


// Define flags for the peerconnect_client testing tool, in a separate
// header file so that they can be shared across the different main.cc's
// for each platform.

DEFINE_bool(help, false, "Prints this message");
DEFINE_bool(h, false, "Prints this message");
DEFINE_bool(server, false, "server mode");
DEFINE_string(p, "", "password");
DEFINE_string(r, "", "Room id");
DEFINE_bool(udp, false, "UDP mode");


#endif  // HOTLINE_TUNNEL_FLAGDEFS_H_
