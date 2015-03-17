#ifndef TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#pragma once

#include "htn_config.h"

#include <deque>
#include <map>
#include <set>
#include <string>

#include "webrtc/base/scoped_ptr.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "main_wnd.h"
#include "peer_connection_client.h"
#include "socket_connection.h"


namespace webrtc {
class VideoCaptureModule;
}  // namespace webrtc

namespace cricket {
class VideoRenderer;
}  // namespace cricket


class HotineDataChannelObserver;


class Conductor
  : public webrtc::PeerConnectionObserver,
    public webrtc::CreateSessionDescriptionObserver,
    public SocketConnectionObserver,
    public PeerConnectionClientObserver,
    public sigslot::has_slots<>,
    public MainWndCallback {
 public:
  enum CallbackID {
    MEDIA_CHANNELS_INITIALIZED = 1,
    PEER_CONNECTION_CLOSED,
    SEND_MESSAGE_TO_PEER,
    NEW_STREAM_ADDED,
    STREAM_REMOVED,
  };

  Conductor(PeerConnectionClient* client, SocketConnection *socket_connection, MainWindow* main_wnd);

  bool connection_active() const;

  virtual void Close();

 protected:
  ~Conductor();
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  void EnsureStreamingUI();
  void AddDataChannels(std::string& channel_name);

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
  // SocketListeningObserver implementation.
  //

  virtual void OnSocketConnected();
  virtual void OnSocketDisconnected() {};
  virtual void OnMessageFromSocket() {};
  virtual void OnMessageToSocket() {};

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

  // For HotlineDataChannelObserver
  void OnMainDatachannelReady();

 protected:
  // Send a message to the remote peer.
  void SendMessage(const std::string& json_object);

  int peer_id_;
  bool loopback_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory_;
  PeerConnectionClient* client_;
  SocketConnection* socket_connection_;
  MainWindow* main_wnd_;
  std::deque<std::string*> pending_messages_;

  rtc::scoped_refptr<HotineDataChannelObserver> main_datachannel_;
  std::map < std::string, rtc::scoped_refptr<HotineDataChannelObserver> >
      local_datachannels_;
  std::map < std::string, rtc::scoped_refptr<HotineDataChannelObserver> >
      remote_datachannels_;

  long local_datachannel_serial_;
  long remote_datachannel_serial_;

  std::string server_;
};





#endif  // TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
