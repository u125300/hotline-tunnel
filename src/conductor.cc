#include "htn_config.h"
#include "conductor.h"

#include <utility>
#include <vector>

#include "talk/app/webrtc/videosourceinterface.h"
#include "talk/media/devices/devicemanager.h"
#include "talk/app/webrtc/test/fakeconstraints.h"
#include "webrtc/base/common.h"
#include "webrtc/base/json.h"
#include "webrtc/base/logging.h"
#include "defaults.h"
#include "data_channel.h"


// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

#define DTLS_ON  true
#define DTLS_OFF false

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return
        new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() {
    LOG(INFO) << __FUNCTION__;
  }
  virtual void OnFailure(const std::string& error) {
    LOG(INFO) << __FUNCTION__ << " " << error;
  }

 protected:
  DummySetSessionDescriptionObserver() {}
  ~DummySetSessionDescriptionObserver() {}
};

Conductor::Conductor(PeerConnectionClient* client, SocketConnection *socket_connection, MainWindow* main_wnd)
  : peer_id_(-1),
    loopback_(false),
    client_(client),
    socket_connection_(socket_connection),
    local_datachannel_serial_(1),
    remote_datachannel_serial_(1),

    main_wnd_(main_wnd) {
  client_->RegisterObserver(this);
  socket_connection_->RegisterObserver(this);
  main_wnd->RegisterObserver(this);
}

Conductor::~Conductor() {
  ASSERT(peer_connection_.get() == NULL);
}

bool Conductor::connection_active() const {
  return peer_connection_.get() != NULL;
}

void Conductor::Close() {
  client_->SignOut();
  DeletePeerConnection();
}

bool Conductor::InitializePeerConnection() {
  ASSERT(peer_connection_factory_.get() == NULL);
  ASSERT(peer_connection_.get() == NULL);

  peer_connection_factory_  = webrtc::CreatePeerConnectionFactory();

  if (!peer_connection_factory_.get()) {
    main_wnd_->MessageBox("Error",
        "Failed to initialize PeerConnectionFactory", true);
    DeletePeerConnection();
    return false;
  }

  if (!CreatePeerConnection(DTLS_ON)) {
    main_wnd_->MessageBox("Error",
        "CreatePeerConnection failed", true);
    DeletePeerConnection();
  }
  
  AddDataChannels(std::string(kMainDataLabel));
  main_wnd_->SwitchToStreamingUI();

  return peer_connection_.get() != NULL;
}

bool Conductor::ReinitializePeerConnectionForLoopback() {
  loopback_ = true;
  rtc::scoped_refptr<webrtc::StreamCollectionInterface> streams(
      peer_connection_->local_streams());
  peer_connection_ = NULL;
  if (CreatePeerConnection(DTLS_OFF)) {
    for (size_t i = 0; i < streams->count(); ++i)
      peer_connection_->AddStream(streams->at(i));
    peer_connection_->CreateOffer(this, NULL);
  }
  return peer_connection_.get() != NULL;
}

bool Conductor::CreatePeerConnection(bool dtls) {
  ASSERT(peer_connection_factory_.get() != NULL);
  ASSERT(peer_connection_.get() == NULL);

  webrtc::PeerConnectionInterface::IceServers servers;
  webrtc::PeerConnectionInterface::IceServer server;
  server.uri = GetPeerConnectionString();
  servers.push_back(server);

  webrtc::FakeConstraints constraints;
  if (dtls) {
    constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
                            "true");
  }

  peer_connection_ =
      peer_connection_factory_->CreatePeerConnection(servers,
                                                     &constraints,
                                                     NULL,
                                                     NULL,
                                                     this);
  return peer_connection_.get() != NULL;
}

void Conductor::DeletePeerConnection() {
  peer_connection_ = NULL;
  local_datachannels_.clear();
  remote_datachannels_.clear();
  main_wnd_->StopLocalRenderer();
  main_wnd_->StopRemoteRenderer();
  peer_connection_factory_ = NULL;
  peer_id_ = -1;
  loopback_ = false;
}

void Conductor::EnsureStreamingUI() {
  ASSERT(peer_connection_.get() != NULL);
  if (main_wnd_->IsWindow()) {
    if (main_wnd_->current_ui() != MainWindow::STREAMING)
      main_wnd_->SwitchToStreamingUI();
  }
}

//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Conductor::OnAddStream(webrtc::MediaStreamInterface* stream) {
  LOG(INFO) << __FUNCTION__ << " " << stream->label();

  stream->AddRef();
  main_wnd_->QueueUIThreadCallback(NEW_STREAM_ADDED,
                                   stream);
}

void Conductor::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
  LOG(INFO) << __FUNCTION__ << " " << stream->label();
  stream->AddRef();
  main_wnd_->QueueUIThreadCallback(STREAM_REMOVED,
                                   stream);
}

// Remote peer created data channel.
// So, store it to recv_datachannels.
void Conductor::OnDataChannel(webrtc::DataChannelInterface* channel) {
  LOG(INFO) << __FUNCTION__;

  rtc::scoped_refptr<HotineDataChannelObserver> data_channel_observer(
    new rtc::RefCountedObject<HotineDataChannelObserver>(channel, false));

  typedef std::pair<std::string,
    rtc::scoped_refptr<HotineDataChannelObserver> >
    DataChannelObserverPair;

  remote_datachannels_.insert(DataChannelObserverPair(channel->label(), data_channel_observer));
}



void Conductor::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  LOG(INFO) << __FUNCTION__ << " " << candidate->sdp_mline_index();
  // For loopback test. To save some connecting delay.
  if (loopback_) {
    if (!peer_connection_->AddIceCandidate(candidate)) {
      LOG(WARNING) << "Failed to apply the received candidate";
    }
    return;
  }

  Json::StyledWriter writer;
  Json::Value jmessage;

  jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }
  jmessage[kCandidateSdpName] = sdp;
  SendMessage(writer.write(jmessage));
}

//
// SocketClientObserver implementation.
//

void Conductor::OnSocketConnected() {

  std::string serial = std::to_string(local_datachannel_serial_++);
  AddDataChannels(kDataPrefixLabel+std::string(serial));

}




//
// PeerConnectionClientObserver implementation.
//

void Conductor::OnSignedIn() {
  LOG(INFO) << __FUNCTION__;
  main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::OnDisconnected() {
  LOG(INFO) << __FUNCTION__;

  DeletePeerConnection();

  if (main_wnd_->IsWindow())
    main_wnd_->SwitchToConnectUI();
}

void Conductor::OnPeerConnected(int id, const std::string& name) {
  LOG(INFO) << __FUNCTION__;
  // Refresh the list if we're showing it.
  if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
    main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::OnPeerDisconnected(int id) {
  LOG(INFO) << __FUNCTION__;
  if (id == peer_id_) {
    LOG(INFO) << "Our peer disconnected";
    main_wnd_->QueueUIThreadCallback(PEER_CONNECTION_CLOSED, NULL);
  } else {
    // Refresh the list if we're showing it.
    if (main_wnd_->current_ui() == MainWindow::LIST_PEERS)
      main_wnd_->SwitchToPeerList(client_->peers());
  }
}

void Conductor::OnMessageFromPeer(int peer_id, const std::string& message) {
  ASSERT(peer_id_ == peer_id || peer_id_ == -1);
  ASSERT(!message.empty());

  if (!peer_connection_.get()) {
    ASSERT(peer_id_ == -1);
    peer_id_ = peer_id;

    if (!InitializePeerConnection()) {
      LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
      client_->SignOut();
      return;
    }
  } else if (peer_id != peer_id_) {
    ASSERT(peer_id_ != -1);
    LOG(WARNING) << "Received a message from unknown peer while already in a "
                    "conversation with a different peer.";
    return;
  }

  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(message, jmessage)) {
    LOG(WARNING) << "Received unknown message. " << message;
    return;
  }
  std::string type;
  std::string json_object;

  GetStringFromJsonObject(jmessage, kSessionDescriptionTypeName, &type);
  if (!type.empty()) {
    if (type == "offer-loopback") {
      // This is a loopback call.
      // Recreate the peerconnection with DTLS disabled.
      if (!ReinitializePeerConnectionForLoopback()) {
        LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
        DeletePeerConnection();
        client_->SignOut();
      }
      return;
    }

    std::string sdp;
    if (!GetStringFromJsonObject(jmessage, kSessionDescriptionSdpName, &sdp)) {
      LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription(type, sdp));
    if (!session_description) {
      LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
    LOG(INFO) << " Received session description :" << message;
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(), session_description);
    if (session_description->type() ==
        webrtc::SessionDescriptionInterface::kOffer) {
      peer_connection_->CreateAnswer(this, NULL);
    }
    return;
  } else {
    std::string sdp_mid;
    int sdp_mlineindex = 0;
    std::string sdp;
    if (!GetStringFromJsonObject(jmessage, kCandidateSdpMidName, &sdp_mid) ||
        !GetIntFromJsonObject(jmessage, kCandidateSdpMlineIndexName,
                              &sdp_mlineindex) ||
        !GetStringFromJsonObject(jmessage, kCandidateSdpName, &sdp)) {
      LOG(WARNING) << "Can't parse received message.";
      return;
    }
    rtc::scoped_ptr<webrtc::IceCandidateInterface> candidate(
        webrtc::CreateIceCandidate(sdp_mid, sdp_mlineindex, sdp));
    if (!candidate.get()) {
      LOG(WARNING) << "Can't parse received candidate message.";
      return;
    }
    if (!peer_connection_->AddIceCandidate(candidate.get())) {
      LOG(WARNING) << "Failed to apply the received candidate";
      return;
    }
    LOG(INFO) << " Received candidate :" << message;
    return;
  }
}

void Conductor::OnMessageSent(int err) {
  // Process the next pending message if any.
  main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, NULL);
}

void Conductor::OnServerConnectionFailure() {
    main_wnd_->MessageBox("Error", ("Failed to connect to " + server_).c_str(),
                          true);
}

//
// MainWndCallback implementation.
//

void Conductor::StartLogin(const std::string& server, int port) {
  if (client_->is_connected())
    return;
  server_ = server;
  client_->Connect(server, port, GetPeerName());
}

void Conductor::DisconnectFromServer() {
  if (client_->is_connected())
    client_->SignOut();
}

void Conductor::ConnectToPeer(int peer_id) {
  ASSERT(peer_id_ == -1);
  ASSERT(peer_id != -1);

  if (peer_connection_.get()) {
    main_wnd_->MessageBox("Error",
        "We only support connecting to one peer at a time", true);
    return;
  }

  if (InitializePeerConnection()) {
    peer_id_ = peer_id;
    peer_connection_->CreateOffer(this, NULL);
  } else {
    main_wnd_->MessageBox("Error", "Failed to initialize PeerConnection", true);
  }
}


//
// Add data channels to send
//

void Conductor::AddDataChannels(std::string& channel_name) {

  typedef std::pair<std::string,
                    rtc::scoped_refptr<HotineDataChannelObserver> >
                    DataChannelObserverPair;

  if (channel_name == kMainDataLabel) {
    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel =
      peer_connection_->CreateDataChannel(channel_name, NULL);

    if (data_channel.get() == NULL) {
      LOG(LS_ERROR) << "CreateDataChannel(" + channel_name +") to PeerConnection failed";
      return;
    }

    rtc::scoped_refptr<HotineDataChannelObserver> data_channel_observer(
      new rtc::RefCountedObject<HotineDataChannelObserver>(data_channel, true));

    main_datachannel_ = data_channel_observer;
    main_datachannel_->SignalOpenEvent.connect(this, &Conductor::OnMainDatachannelReady);


  }
  else {
    if (local_datachannels_.find(channel_name) != local_datachannels_.end())
      return;  // Already added.

    rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel =
      peer_connection_->CreateDataChannel(channel_name, NULL);

    if (data_channel.get() == NULL) {
      LOG(LS_ERROR) << "CreateDataChannel to PeerConnection failed";
      return;
    }

    rtc::scoped_refptr<HotineDataChannelObserver> data_channel_observer(
      new rtc::RefCountedObject<HotineDataChannelObserver>(data_channel, false));

    local_datachannels_.insert(DataChannelObserverPair(data_channel->label(),
                               data_channel_observer));
  }
}


void Conductor::DisconnectFromCurrentPeer() {
  LOG(INFO) << __FUNCTION__;
  if (peer_connection_.get()) {
    client_->SendHangUp(peer_id_);
    DeletePeerConnection();
  }

  if (main_wnd_->IsWindow())
    main_wnd_->SwitchToPeerList(client_->peers());
}

void Conductor::UIThreadCallback(int msg_id, void* data) {
  switch (msg_id) {
    case PEER_CONNECTION_CLOSED:
      LOG(INFO) << "PEER_CONNECTION_CLOSED";
      DeletePeerConnection();

      ASSERT(local_datachannels_.empty());
      ASSERT(remote_datachannels_.empty());

      if (main_wnd_->IsWindow()) {
        if (client_->is_connected()) {
          main_wnd_->SwitchToPeerList(client_->peers());
        } else {
          main_wnd_->SwitchToConnectUI();
        }
      } else {
        DisconnectFromServer();
      }
      break;

    case SEND_MESSAGE_TO_PEER: {
      LOG(INFO) << "SEND_MESSAGE_TO_PEER";
      std::string* msg = reinterpret_cast<std::string*>(data);
      if (msg) {
        // For convenience, we always run the message through the queue.
        // This way we can be sure that messages are sent to the server
        // in the same order they were signaled without much hassle.
        pending_messages_.push_back(msg);
      }

      if (!pending_messages_.empty() && !client_->IsSendingMessage()) {
        msg = pending_messages_.front();
        pending_messages_.pop_front();

        if (!client_->SendToPeer(peer_id_, *msg) && peer_id_ != -1) {
          LOG(LS_ERROR) << "SendToPeer failed";
          DisconnectFromServer();
        }
        delete msg;
      }

      if (!peer_connection_.get())
        peer_id_ = -1;

      break;
    }

    case NEW_STREAM_ADDED: {
      webrtc::MediaStreamInterface* stream =
          reinterpret_cast<webrtc::MediaStreamInterface*>(
          data);
      webrtc::VideoTrackVector tracks = stream->GetVideoTracks();
      // Only render the first track.
      if (!tracks.empty()) {
        webrtc::VideoTrackInterface* track = tracks[0];
        main_wnd_->StartRemoteRenderer(track);
      }
      stream->Release();
      break;
    }

    case STREAM_REMOVED: {
      // Remote peer stopped sending a stream.
      webrtc::MediaStreamInterface* stream =
          reinterpret_cast<webrtc::MediaStreamInterface*>(
          data);
      stream->Release();
      break;
    }

    default:
      ASSERT(false);
      break;
  }
}

void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create(), desc);

  std::string sdp;
  desc->ToString(&sdp);

  // For loopback test. To save some connecting delay.
  if (loopback_) {
    // Replace message type from "offer" to "answer"
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription("answer", sdp));
    peer_connection_->SetRemoteDescription(
        DummySetSessionDescriptionObserver::Create(), session_description);
    return;
  }

  Json::StyledWriter writer;
  Json::Value jmessage;
  jmessage[kSessionDescriptionTypeName] = desc->type();
  jmessage[kSessionDescriptionSdpName] = sdp;
  SendMessage(writer.write(jmessage));
}

void Conductor::OnFailure(const std::string& error) {
    LOG(LERROR) << error;
}


void Conductor::OnMainDatachannelReady() {

  //
  // Start listen if client mode
  //

  LOG(INFO) << "Main data channel opened.";

  if (socket_connection_ && socket_connection_->client_mode()) {
    if (socket_connection_->StartClientMode()){
      // TODO: exit, Not implelemted yet
    }
  }

}


void Conductor::SendMessage(const std::string& json_object) {
  std::string* msg = new std::string(json_object);
  main_wnd_->QueueUIThreadCallback(SEND_MESSAGE_TO_PEER, msg);
}


