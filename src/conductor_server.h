#ifndef HOTLINE_TUNNEL_CONDUCTOR_SERVER_H_
#define HOTLINE_TUNNEL_CONDUCTOR_SERVER_H_
#pragma once

#include "htn_config.h"

#include <map>
#include <string>

#include "conductor.h"


namespace hotline {


class ConductorServer
  : public Conductor {
public:

 ConductorServer(SignalServerConnection* signal_client,
            UserArguments& arguments);

protected:

  ~ConductorServer();

};

} // namespace hotline



#endif  // HOTLINE_TUNNEL_CONDUCTOR_SERVER_H_
