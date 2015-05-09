#ifndef HOTLINE_TUNNEL_CONDUCTOR_H_
#define HOTLINE_TUNNEL_CONDUCTOR_H_
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


namespace hotline {

class Conductor
  : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver,
    public HotlineDataChannelObserver,
    public SocketObserver,
    public rtc::MessageHandler {
 public:
  enum ThreadMsgId{
    MsgStopLane
  };

  Conductor::Conductor();
  virtual ~Conductor();

  void Conductor::Init( bool server_mode,
                        rtc::SocketAddress local_address,
                        rtc::SocketAddress remote_address,
                        cricket::ProtocolType protocol,
                        std::string room_id,
                        uint64 local_peer_id,
                        uint64 remote_peer_id,
                        SignalServerConnection* signal_client,
                        rtc::Thread* signal_thread
                        );

  bool connection_active() const;
  void ConnectToPeer();
  virtual void OnReceivedOffer(Json::Value& data);
  virtual void Close();
  
 protected:
   class ChannelDescription{
   public:
     ChannelDescription() {}
     
     void Set(rtc::SocketAddress remote_address, cricket::ProtocolType protocol) {
       remote_address_ = remote_address;
       protocol_ = protocol;
     }
                 
     rtc::SocketAddress& remote_address() { return remote_address_;}
     cricket::ProtocolType protocol() { return protocol_; }
   private:
     rtc::SocketAddress remote_address_;
     cricket::ProtocolType protocol_;
  };

  struct LaneMessageData : public rtc::MessageData {
  public:
    LaneMessageData(SocketConnection* socket_connection, HotlineDataChannel* data_channel) {
      socket_connection_ = socket_connection;
      data_channel_ = data_channel;
    }

    SocketConnection* socket_connection() { return socket_connection_;}
    HotlineDataChannel* data_channel() { return data_channel_; }

  private:
    SocketConnection* socket_connection_;
    HotlineDataChannel* data_channel_;
  };

  
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  bool AddControlDataChannel();
  bool AddPacketDataChannel(std::string* channel_name);

  // create client socket + data channel + server socket connection
  bool CreateConnectionLane(SocketConnection* connection);
  bool CreateConnectionLane(rtc::scoped_refptr<HotlineDataChannel> channel);
  // delete client socket + data channel + server socket connection
  void DeleteConnectionLane(SocketConnection* connection, rtc::scoped_refptr<HotlineDataChannel> channel);

  bool server_mode() { return server_mode_; }
  bool client_mode() { return !server_mode_; }

  //
  // PeerConnectionObserver implementation.
  //
  virtual void OnStateChange(
      webrtc::PeerConnectionObserver::StateType state_changed) {}
  virtual void OnAddStream(webrtc::MediaStreamInterface* stream);
  virtual void OnRemoveStream(webrtc::MediaStreamInterface* stream);
  virtual void OnDataChannel(webrtc::DataChannelInterface* channel);
  virtual void OnRenegotiationNeeded() {}
  virtual void OnIceChange() {}
  virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate);

  //
  // HotlineDataChannelObserver implementation.
  //
  virtual void OnControlDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel, bool is_local);
  virtual void OnControlDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel, bool is_local);
  virtual void OnSocketDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel);
  virtual void OnSocketDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel);
  virtual void OnCreateChannel(rtc::SocketAddress& remote_address, cricket::ProtocolType protocol);
  virtual void OnStopChannel(std::string& channel_name);
  virtual void OnChannelCreated();
  virtual void OnServerSideReady(std::string& channel_name);

  //
  // SocketObserver implementation.
  //
  virtual void OnSocketOpen(SocketConnection* socket);
  virtual void OnSocketClosed(SocketConnection* socket);
  virtual void OnSocketStop(SocketConnection* socket);

  //
  // CreateSessionDescriptionObserver implementation.
  //
  virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
  virtual void OnFailure(const std::string& error);

  //
  // implements the MessageHandler interface
  //
  void OnMessage(rtc::Message* msg);


  //
  // Local variables
  //

  uint64 local_peer_id_;
  uint64 remote_peer_id_;
  bool loopback_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  SignalServerConnection* signal_client_;
  std::deque<std::string*> pending_messages_;

  rtc::scoped_refptr<HotlineControlDataChannel> local_control_datachannel_;
  rtc::scoped_refptr<HotlineControlDataChannel> remote_control_datachannel_;
  std::map < std::string, rtc::scoped_refptr<HotlineDataChannel> >
      datachannels_;

  long local_datachannel_serial_;

  SocketListenServer socket_listen_server_;
  SocketClient socket_client_;
  bool server_mode_;
  rtc::SocketAddress local_address_;
  rtc::SocketAddress remote_address_;
  std::string room_id_;
  cricket::ProtocolType protocol_;

  rtc::Thread* signal_thread_;
  ChannelDescription channel_;
};

} // namespace hotline



#endif  // HOTLINE_TUNNEL_CONDUCTOR_H_
