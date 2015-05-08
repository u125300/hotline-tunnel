#include "htn_config.h"
#include "defaults.h"

#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#else
#include <unistd.h>
#endif

#include "webrtc/base/common.h"

const char kControlDataLabel[] = "control_label";
const char kDefaultServerPath[] = "htunnel";

std::string GetEnvVarOrDefault(const char* env_var_name,
                               const char* default_value) {
  std::string value;
  const char* env_var = getenv(env_var_name);
  if (env_var)
    value = env_var;

  if (value.empty())
    value = default_value;

  return value;
}

std::string GetPeerConnectionString() {
  return GetEnvVarOrDefault("HOTLINE_WEBRTC_CONNECT", "stun:stun.l.google.com:19302");
}

std::string GetSignalServerName() {
  return GetEnvVarOrDefault("HOTLINE_SIGNAL_SERVER", "ws://127.0.0.1:8888");
}
