#include "htn_config.h"
#include "conductor_server.h"

namespace hotline {

ConductorServer::ConductorServer(SignalServerConnection* signal_client,
                     UserArguments& arguments)
  : Conductor(signal_client, arguments)
{
}

ConductorServer::~ConductorServer() 
{
}

} // namespace hotline
