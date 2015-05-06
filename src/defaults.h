#ifndef HOTLINE_TUNNEL_DEFAULTS_H_
#define HOTLINE_TUNNEL_DEFAULTS_H_
#pragma once

#include "htn_config.h"

#include <string>

#include "webrtc/base/basictypes.h"

extern const char kAudioLabel[];
extern const char kVideoLabel[];
extern const char kStreamLabel[];
extern const char kControlDataLabel[];
extern const char kDataPrefixLabel[];
extern const char kDefaultServerPath[];
extern const uint16 kDefaultServerPort;

std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value);
std::string GetPeerConnectionString();
std::string GetDefaultServerName();
std::string GetPeerName();

#endif  // HOTLINE_TUNNEL_DEFAULTS_H_
