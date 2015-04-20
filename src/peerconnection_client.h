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

  virtual void OnSignedIn2() = 0;  // Called when we're logged on.
  virtual void OnDisconnected2() = 0;
  virtual void OnServerConnectionFailure2() = 0;

 protected:
  virtual ~PeerConnectionClientObserver2() {}

};


class PeerConnectionClient2 : public sigslot::has_slots<>,
                             public rtc::MessageHandler {
public:
  PeerConnectionClient2(bool client_mode);
  virtual ~PeerConnectionClient2();

  void RegisterObserver(PeerConnectionClientObserver2* callback);
  void Connect(const std::string& url);

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

protected:
  void InitSocketSignals();
  virtual void onOpen(WebSocket* ws);
  virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
  virtual void onClose(WebSocket* ws);
  virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

private:

  bool StartClientMode(int id);
  bool StartServerMode();

  void created(Json::Value& data);
  void joined(Json::Value& data);

  template<typename  T>
  bool Send(const std::string msgid, T& data);

  //
  // Member variables
  //

  int id_;
  rtc::scoped_ptr<WebSocket> ws_;
  bool server_mode_;
};


//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_PEERCONNECTION_CLIENT_H_
