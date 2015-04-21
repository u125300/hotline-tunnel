#include "htn_config.h"
#include "conductor_client.h"

namespace hotline {


ConductorClient::ConductorClient(SignalServerConnection* signal_client,
                     UserArguments& arguments)
  : Conductor(signal_client, arguments)
{
}

ConductorClient::~ConductorClient() 
{
}



} // namespace hotline
