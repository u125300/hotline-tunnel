#ifndef TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#pragma once

#include "htn_config.h"

#include <map>
#include <string>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/p2p/base/portinterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "main_wnd.h"
#include "data_channel.h"
#include "peer_connection_client.h"
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
  std::string tunnel_key;
};


class Conductor
  : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver,
    public HotlineDataChannelObserver,
    public SocketObserver,
    public PeerConnectionClientObserver,
    public MainWndCallback {
 public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_STREAM_ADDED,
    STREAM_REMOVED,
  };

  Conductor(PeerConnectionClient* client,
            UserArguments& arguments,
            MainWindow* main_wnd);

  bool connection_active() const;

  virtual void Close();

 protected:
  ~Conductor();
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  void EnsureStreamingUI();
  bool AddDataChannels(std::string& channel_name);

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
  virtual void OnControlDataChannelOpen(bool is_local);
  virtual void OnControlDataChannelClosed(bool is_local);
  virtual void OnSocketDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel);
  virtual void OnSocketDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel);

  //
  // SocketObserver implementation.
  //
  virtual void OnSocketOpen(SocketConnection* socket);
  virtual void OnSocketClosed(SocketConnection* socket);

  //
  // PeerConnectionClientObserver implementation.
  //
  virtual void OnSignedIn();

  virtual void OnDisconnected();

  virtual void OnPeerConnected(int id, const std::string& name);

  virtual void OnPeerDisconnected(int id);

  virtual void OnMessageFromPeer(int peer_id, const std::string& message);

  virtual void OnMessageSent(int err);

  virtual void OnServerConnectionFailure();

  //
  // MainWndCallback implementation.
  //

  virtual void StartLogin(const std::string& server, int port);

  virtual void DisconnectFromServer();

  virtual void ConnectToPeer(int peer_id);

  virtual void DisconnectFromCurrentPeer();

  virtual void UIThreadCallback(int msg_id, void* data);

  // CreateSessionDescriptionObserver implementation.
  virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc);
  virtual void OnFailure(const std::string& error);

 protected:
  // Send a message to the remote peer.
  void SendMessage(const std::string& json_object);

  int peer_id_;
  bool loopback_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionClient* client_;
  MainWindow* main_wnd_;
  std::deque<std::string*> pending_messages_;

  rtc::scoped_refptr<HotlineControlDataChannel> local_control_datachannel_;
  rtc::scoped_refptr<HotlineControlDataChannel> remote_control_datachannel_;
  std::map < std::string, rtc::scoped_refptr<HotlineDataChannel> >
      local_datachannels_;
  std::map < std::string, rtc::scoped_refptr<HotlineDataChannel> >
      remote_datachannels_;

  long local_datachannel_serial_;

  hotline::SocketListenServer socket_listen_server_;
  hotline::SocketClient socket_client_;
  bool server_mode_;
  rtc::SocketAddress local_address_;
  rtc::SocketAddress remote_address_;
  std::string tunnel_key_;
  cricket::ProtocolType protocol_;

  std::string server_;
};

} // namespace hotline



#endif  // TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
