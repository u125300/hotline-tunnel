#include "htn_config.h"
#include "conductor_server.h"

namespace hotline {

ConductorServer::ConductorServer(PeerConnectionClient* client,
                     UserArguments& arguments)
  : Conductor(client, arguments)
{
}

ConductorServer::~ConductorServer() 
{
}

} // namespace hotline
