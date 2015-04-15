#ifndef HOTLINE_TUNNEL_PEERCONNECTION_CLIENT_H_
#define HOTLINE_TUNNEL_PEERCONNECTION_CLIENT_H_
#pragma once

#include <string>
#include <vector>

#include "webrtc/base/sigslot.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/scoped_ptr.h"
#include "websocket.h"


namespace hotline {

struct PeerConnectionClientObserver2 {

 protected:
  virtual ~PeerConnectionClientObserver2() {}

};


class PeerConnectionClient2 : public sigslot::has_slots<>,
                             public rtc::MessageHandler {
public:
  PeerConnectionClient2();
  virtual ~PeerConnectionClient2();

  void RegisterObserver(PeerConnectionClientObserver2* callback);
  void Connect(const std::string& url);

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

protected:
  void InitSocketSignals();

private:
  virtual void onOpen(WebSocket* ws);
  virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
  virtual void onClose(WebSocket* ws);
  virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

  //
  // Member variables
  //

  rtc::scoped_ptr<WebSocket> ws_;
};


//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_PEERCONNECTION_CLIENT_H_
