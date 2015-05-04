#ifndef HOTLINE_TUNNEL_SIGNALSERVER_CONNECTION_H_
#define HOTLINE_TUNNEL_SIGNALSERVER_CONNECTION_H_
#pragma once

#include <string>
#include <vector>

#include "webrtc/base/sigslot.h"
#include "webrtc/base/messagehandler.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/json.h"
#include "websocket.h"


namespace hotline {

struct SignalServerConnectionObserver {

  virtual void OnConnected() = 0;
  virtual void OnDisconnected() = 0;
  virtual void OnCreatedRoom(std::string& room_id) = 0;
  virtual void OnSignedIn(std::string& room_id, uint64 peer_id) = 0;
  virtual void OnPeerConnected(uint64 peer_id) = 0;
  virtual void OnReceivedOffer(Json::Value& data) = 0;
  virtual void OnMessageFromPeer() = 0;
  virtual void OnMessageSent() = 0;
  virtual void OnServerConnectionFailure() = 0;

protected:
  virtual ~SignalServerConnectionObserver() {}

};


class SignalServerConnection : public sigslot::has_slots<>,
                             public rtc::MessageHandler {
public:

  // MsgID must be the same as MsgId of signal_server.py
  enum MsgID {
    MsgCreateRoom        = 1,
    MsgSignIn            = 2,
    MsgPeerConnected     = 3,
    MsgSendOffer         = 4,
    MsgReceivedOffer     = 5
  };

  enum ThreadMsgId{
    MsgServerMessage
  };

  struct ServerMessageData : public rtc::MessageData {
  public:
    ServerMessageData(MsgID msgid, Json::Value& data) {
      msgid_ = msgid;
      data_ = data;
    }

    MsgID msgid() {return msgid_;}
    Json::Value& data() {return data_;}

  private:
    MsgID msgid_;
    Json::Value data_;
  };

  SignalServerConnection(rtc::Thread* signal_thread);
  virtual ~SignalServerConnection();

  void Connect(const std::string& url);
  void CreateRoom(const std::string& password);
  void SignIn(std::string& room_id, std::string& password);

  void RegisterObserver(SignalServerConnectionObserver* callback);
  void UnregisterObserver(SignalServerConnectionObserver* callback);

  template<typename  T>
  bool Send(const MsgID msgid, T& data);
  bool Send(const MsgID msgid);

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg);

protected:
  virtual void onOpen(WebSocket* ws);
  virtual void onMessage(WebSocket* ws, const WebSocket::Data& data);
  virtual void onClose(WebSocket* ws);
  virtual void onError(WebSocket* ws, const WebSocket::ErrorCode& error);

  virtual void OnCreatedRoom(Json::Value& data);
  virtual void OnSignedIn(Json::Value& data);
  virtual void OnPeerConnected(Json::Value& data);
  virtual void OnReceivedOffer(Json::Value& data);


private:
  void InitSocketSignals();
  void ServerConnectionFailure();

  //
  // Member variables
  //

  rtc::scoped_ptr<WebSocket> ws_;
  SignalServerConnectionObserver* callback_;
  rtc::Thread *signal_thread_;
  std::string password_;
};


//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_SIGNALSERVER_CONNECTION_H_
