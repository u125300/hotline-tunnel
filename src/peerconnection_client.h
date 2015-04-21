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

struct PeerConnectionClientObserver {

  virtual void OnSignedIn(std::string& room_id) = 0;
  virtual void OnDisconnected() = 0;
  virtual void OnMessageFromPeer() = 0;
  virtual void OnMessageSent() = 0;
  virtual void OnServerConnectionFailure() = 0;

 protected:
  virtual ~PeerConnectionClientObserver() {}

};


class PeerConnectionClient : public sigslot::has_slots<>,
                             public rtc::MessageHandler {
public:

  enum MsgID {
    SigninAsServer  = 1,
    SigninAsClient    = 2,
    SendMessageToPeer = 3
  };

  PeerConnectionClient(rtc::Thread* signal_thread);
  virtual ~PeerConnectionClient();

  bool InitClientMode(std::string& password, std::string& room_id);
  bool InitServerMode(std::string& password);

  void RegisterObserver(PeerConnectionClientObserver* callback);
  void Connect(const std::string& url);

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

  template<typename  T>
  bool Send(const MsgID msgid, T& data);

protected:
  void InitSocketSignals();
  virtual void onOpen(WebSocket* ws);
  virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
  virtual void onClose(WebSocket* ws);
  virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

private:

  bool StartClientMode();
  bool StartServerMode();

  void SignedInAsServer(Json::Value& data);
  void SignedInAsClient(Json::Value& data);
  void Exit();


  //
  // Member variables
  //

  std::string room_id_;
  uint64 peer_id_;
  rtc::scoped_ptr<WebSocket> ws_;
  bool server_mode_;
  std::string password_;
  PeerConnectionClientObserver* callback_;
  rtc::Thread *signal_thread_;
};


//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_PEERCONNECTION_CLIENT_H_
