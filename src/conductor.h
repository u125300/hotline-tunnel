#ifndef TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#define TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
#pragma once

#include "htn_config.h"

#include <deque>
#include <map>
#include <set>
#include <string>

#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "main_wnd.h"
#include "peer_connection_client.h"
#include "webrtc/base/scoped_ptr.h"

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

  Conductor(PeerConnectionClient* client, MainWindow* main_wnd);

  bool connection_active() const;

  virtual void Close();

 protected:
  ~Conductor();
  bool InitializePeerConnection();
  bool ReinitializePeerConnectionForLoopback();
  bool CreatePeerConnection(bool dtls);
  void DeletePeerConnection();
  void EnsureStreamingUI();
  void AddDataChannels();
  cricket::VideoCapturer* OpenVideoCaptureDevice();

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
  std::map < std::string, rtc::scoped_refptr<HotineDataChannelObserver> >
      send_datachannels_;
  std::map < std::string, rtc::scoped_refptr<HotineDataChannelObserver> >
      recv_datachannels_;
  std::string server_;
};


// HotlineDataChnnelObserver
// Observe data channel statechange including open, close and message incoming from peer.
// Create instance per data channel because OnStateChange() and OnMessage() has 
// no input webrtc::DataChannelInterface pointer argument.
//
class HotineDataChannelObserver
  : public webrtc::DataChannelObserver,
    public rtc::RefCountInterface {
public:

  explicit HotineDataChannelObserver(webrtc::DataChannelInterface* channel)
    : channel_(channel), received_message_count_(0) {
    channel_->RegisterObserver(this);
    state_ = channel_->state();
  }

  virtual ~HotineDataChannelObserver() {
    channel_->UnregisterObserver();
  }

  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

  bool IsOpen() const { return state_ == webrtc::DataChannelInterface::kOpen; }
  size_t received_message_count() const { return received_message_count_; }

protected:
  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  webrtc::DataChannelInterface::DataState state_;
  size_t received_message_count_;
};

#endif  // TALK_EXAMPLES_PEERCONNECTION_CLIENT_CONDUCTOR_H_
