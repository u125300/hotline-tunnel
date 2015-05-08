#ifndef HOTLINE_TUNNEL_DEFAULTS_H_
#define HOTLINE_TUNNEL_DEFAULTS_H_
#pragma once

#include "htn_config.h"

#include <string>

#include "webrtc/base/basictypes.h"

extern const char kControlDataLabel[];
extern const char kDefaultServerPath[];

std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value);
std::string GetPeerConnectionString();
std::string GetSignalServerName();

#endif  // HOTLINE_TUNNEL_DEFAULTS_H_
