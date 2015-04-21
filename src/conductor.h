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


namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket


namespace hotline {

struct UserArguments{
  bool server_mode;
  rtc::SocketAddress local_address;
  rtc::SocketAddress remote_address;
  cricket::ProtocolType protocol;
  std::string room_id;
  std::string password;
};


class Conductor
  : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver,
    public HotlineDataChannelObserver,
    public SocketObserver,
    public SignalServerConnectionObserver {
 public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_STREAM_ADDED,
    STREAM_REMOVED,
  };

  Conductor(SignalServerConnection* signal_client,
            UserArguments& arguments);
  virtual ~Conductor();

  bool connection_active() const;

  uint64 id() {return id_;}
  std::string id_string() const;
  static uint64 Conductor::id_from_string(std::string id_string);


  virtual void Close();

 protected:
   class ClientInfo{
   public:
     ClientInfo(uint64 id, rtc::SocketAddress remote_address, cricket::ProtocolType protocol) :
                 id_(id), remote_address_(remote_address), protocol_(protocol){}
     uint64 id() const{return id_;}
     std::string& id_string() const;
     rtc::SocketAddress& remote_address() { return remote_address_;}
     cricket::ProtocolType protocol() { return protocol_; }
   private:
     uint64 id_;
     rtc::SocketAddress remote_address_;
     cricket::ProtocolType protocol_;
  };

  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  void EnsureStreamingUI();
  bool AddDataChannels(std::string& channel_name, bool controlchannel);
  ClientInfo* FindClientInfo(std::string& channel_name);
  void RemoveClientInfo(std::string& channel_name);

  void ConnectToPeer(uint64 peer_id);


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
  virtual void OnCreateClient(uint64 id, rtc::SocketAddress& remote_address, cricket::ProtocolType protocol);
  virtual void OnClientCreated(uint64 id);
  virtual void OnServerSideReady(std::string& channel_name);


  //
  // SocketObserver implementation.
  //
  virtual void OnSocketOpen(SocketConnection* socket);
  virtual void OnSocketClosed(SocketConnection* socket);


  //
  // SignalServerConnectionObserver implementation.
  //

  virtual void OnConnected();
  virtual void OnDisconnected();
  virtual void OnCreatedRoom(std::string& room_id);
  virtual void OnSignedIn(std::string& room_id, uint64 peer_id);
  virtual void OnMessageFromPeer();
  virtual void OnMessageSent();
  virtual void OnServerConnectionFailure();

  // CreateSessionDescriptionObserver implementation.
  virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
  virtual void OnFailure(const std::string& error);


  // Send a message to the remote peer.
  void SendMessage(const std::string& json_object);

  uint64 peer_id_;
  uint64 server_peer_id_;
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
  std::string password_;
  cricket::ProtocolType protocol_;

  uint64 id_;
  std::string server_;

  typedef std::map<uint64, rtc::scoped_ptr<ClientInfo>> ClientMap;
  ClientMap client_conductors_;
};

} // namespace hotline



#endif  // HOTLINE_TUNNEL_CONDUCTOR_H_
