#include "htn_config.h"
#include "conductor_client.h"

namespace hotline {


ConductorClient::ConductorClient(PeerConnectionClient* client,
                     UserArguments& arguments)
  : Conductor(client, arguments)
{
}

ConductorClient::~ConductorClient() 
{
}



} // namespace hotline
