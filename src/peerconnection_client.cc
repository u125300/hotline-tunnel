#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "peerconnection_client.h"

namespace hotline {

PeerConnectionClient2::PeerConnectionClient2()
{
}

PeerConnectionClient2::~PeerConnectionClient2() {
 
}

void PeerConnectionClient2::RegisterObserver(PeerConnectionClientObserver2* callback) {

}


void PeerConnectionClient2::Connect(const std::string& url) {
  ws_ = rtc::scoped_ptr<WebSocket>(new WebSocket());
  if (ws_.get()==NULL) return;

  InitSocketSignals();

  if (!ws_->init(url)) {
    return;
  }

  return;
}


void PeerConnectionClient2::InitSocketSignals() {
  ASSERT(ws_.get() != NULL);

  ws_->SignalConnectEvent.connect(this, &PeerConnectionClient2::onOpen);
  ws_->SignalCloseEvent.connect(this, &PeerConnectionClient2::onClose);
  ws_->SignalErrorEvent.connect(this, &PeerConnectionClient2::onError);
  ws_->SignalReadEvent.connect(this, &PeerConnectionClient2::onMessage);
}


void PeerConnectionClient2::OnMessage(rtc::Message* msg) {
  
}


void PeerConnectionClient2::onOpen(WebSocket* ws) {
  
}
  
void PeerConnectionClient2::onMessage(WebSocket* ws, const WebSocket::Data& data) {
  
}

void PeerConnectionClient2::onClose(WebSocket* ws) {
  
}
  

void PeerConnectionClient2::onError(WebSocket* ws, const WebSocket::ErrorCode& error) {
  
}




///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

