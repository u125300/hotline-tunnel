#ifndef HOTLINE_TUNNEL_CONDUCTOR_CLIENT_H_
#define HOTLINE_TUNNEL_CONDUCTOR_CLIENT_H_
#pragma once

#include "htn_config.h"

#include <map>
#include <string>

#include "conductor.h"


namespace hotline {


class ConductorClient
  : public Conductor {
public:

 ConductorClient(PeerConnectionClient* client,
            UserArguments& arguments);

protected:

  ~ConductorClient();

};

} // namespace hotline



#endif  // HOTLINE_TUNNEL_CONDUCTOR_CLIENT_H_
