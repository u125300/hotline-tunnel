#ifndef HOTLINE_TUNNEL_CONDUCTORS_H_
#define HOTLINE_TUNNEL_CONDUCTORS_H_
#pragma once

#include "htn_config.h"

#include <map>
#include <string>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/p2p/base/portinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "data_channel.h"
#include "signalserver_connection.h"
#include "socket_server.h"
#include "socket_client.h"
#include "conductor.h"


namespace hotline {

struct UserArguments{
  bool server_mode;
  rtc::SocketAddress local_address;
  rtc::SocketAddress remote_address;
  cricket::ProtocolType protocol;
  std::string room_id;
  std::string password;
};


class Conductors
    : public SignalServerConnectionObserver,
      public rtc::MessageHandler {
 public:

   enum ThreadMsgId{
    MsgExit
  };

  Conductors(SignalServerConnection* signal_client,
            rtc::Thread* signal_thread,
            UserArguments& arguments);
  virtual ~Conductors();

  uint64 id() {return id_;}
  std::string id_string() const;
  static uint64 Conductors::id_from_string(std::string id_string);

  virtual void Close();

 protected:

  //
  // SignalServerConnectionObserver implementation.
  //

  virtual void OnConnected();
  virtual void OnDisconnected();
  virtual void OnCreatedRoom(std::string& room_id);
  virtual void OnSignedIn(std::string& room_id, uint64 peer_id);
  virtual void OnPeerConnected(uint64 peer_id);
  virtual void OnPeerDisconnected(uint64 peer_id);
  virtual void OnReceivedOffer(Json::Value& data);
  virtual void OnServerConnectionFailure();

  //
  // implements the MessageHandler interface
  //
  void OnMessage(rtc::Message* msg);
  void OnClose();

  SignalServerConnection* signal_client_;

  bool server_mode_;
  rtc::SocketAddress local_address_;
  rtc::SocketAddress remote_address_;
  cricket::ProtocolType protocol_;
  std::string room_id_;
  std::string password_;

  uint64 id_;
  std::string server_;

  typedef std::map<uint64, rtc::scoped_refptr<Conductor>> PeerMap;
  PeerMap peers_offer_;
  PeerMap peers_answer_;

  rtc::Thread* signal_thread_;
};

} // namespace hotline



#endif  // HOTLINE_TUNNEL_CONDUCTORS_H_
