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

namespace hotline {

// Names used for a IceCandidate JSON object.
const char kCandidateSdpMidName[] = "sdpMid";
const char kCandidateSdpMlineIndexName[] = "sdpMLineIndex";
const char kCandidateSdpName[] = "candidate";

// Names used for a SessionDescription JSON object.
const char kSessionDescriptionTypeName[] = "type";
const char kSessionDescriptionSdpName[] = "sdp";

#define DTLS_ON  true
#define DTLS_OFF false

class HotlineSetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static HotlineSetSessionDescriptionObserver* Create() {
    return
        new rtc::RefCountedObject<HotlineSetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() {
    LOG(INFO) << __FUNCTION__;
  }
  virtual void OnFailure(const std::string& error) {
    LOG(INFO) << __FUNCTION__ << " " << error;
  }

 protected:
  HotlineSetSessionDescriptionObserver() {}
  ~HotlineSetSessionDescriptionObserver() {}
};


Conductor::Conductor()
  : local_peer_id_(0),
    remote_peer_id_(0),
    loopback_(false),
    signal_client_(NULL),
    local_datachannel_serial_(1) {

  socket_client_.RegisterObserver(this);
  socket_listen_server_.RegisterObserver(this);
}

Conductor::~Conductor() {
  Close();
}

void Conductor::Init(bool server_mode,
                    rtc::SocketAddress local_address,
                    rtc::SocketAddress remote_address,
                    cricket::ProtocolType protocol,
                    std::string room_id,
                    uint64 local_peer_id,
                    uint64 remote_peer_id,
                    SignalServerConnection* signal_client,
                    rtc::Thread* signal_thread
                ) {
  server_mode_ = server_mode;
  local_address_ = local_address;
  remote_address_ = remote_address;
  protocol_ = protocol;
  room_id_ = room_id;
  local_peer_id_ = local_peer_id;
  remote_peer_id_ = remote_peer_id;
  signal_client_ = signal_client;
  signal_thread_ = signal_thread;
}

bool Conductor::connection_active() const {
  return peer_connection_.get() != NULL;
}

void Conductor::Close() {
  DeletePeerConnection();
}


bool Conductor::InitializePeerConnection() {
  ASSERT(peer_connection_factory_.get() == NULL);
  ASSERT(peer_connection_.get() == NULL);

  peer_connection_factory_  = webrtc::CreatePeerConnectionFactory();

  if (!peer_connection_factory_.get()) {
    printf("Error: Failed to initialize PeerConnectionFactory\n");
    DeletePeerConnection();
    return false;
  }

  if (!CreatePeerConnection(DTLS_ON)) {
    printf("Error: CreatePeerConnection failed\n");
    DeletePeerConnection();
  }
  
  AddControlDataChannel();

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
  else {
    constraints.AddOptional(webrtc::MediaConstraintsInterface::kEnableDtlsSrtp,
                            "false");
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
  datachannels_.clear();
  local_control_datachannel_ = NULL;
  remote_control_datachannel_ = NULL;

  peer_connection_ = NULL;
  peer_connection_factory_ = NULL;
  local_peer_id_ = 0;
  remote_peer_id_ = 0;
  loopback_ = false;
}


//
// PeerConnectionObserver implementation.
//

// Called when a remote stream is added
void Conductor::OnAddStream(webrtc::MediaStreamInterface* stream) {
  LOG(INFO) << __FUNCTION__ << " " << stream->label();

  stream->AddRef();
}

void Conductor::OnRemoveStream(webrtc::MediaStreamInterface* stream) {
  LOG(INFO) << __FUNCTION__ << " " << stream->label();
  stream->AddRef();
}

// Remote peer created data channel.
// So, store it to recv_datachannels.
void Conductor::OnDataChannel(webrtc::DataChannelInterface* channel) {
  LOG(INFO) << __FUNCTION__;

  if (channel->label().rfind(kControlDataLabel)!=std::string::npos) {
    remote_control_datachannel_ = new rtc::RefCountedObject<HotlineControlDataChannel>(channel, false);
    remote_control_datachannel_->RegisterObserver(this);
  }
  else {
    rtc::scoped_refptr<HotlineDataChannel> data_channel_observer(
      new rtc::RefCountedObject<HotlineDataChannel>(channel, false));
    data_channel_observer->RegisterObserver(this);

    typedef std::pair<std::string,
                      rtc::scoped_refptr<HotlineDataChannel> >
                      DataChannelObserverPair;

    datachannels_.insert(DataChannelObserverPair(channel->label(), data_channel_observer));
  }
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

  Json::FastWriter writer;
  Json::Value jmessage;

  jmessage[kCandidateSdpMidName] = candidate->sdp_mid();
  jmessage[kCandidateSdpMlineIndexName] = candidate->sdp_mline_index();
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }
  jmessage[kCandidateSdpName] = sdp;
  jmessage["room_id"] = room_id_;
  jmessage["peer_id"] = std::to_string(remote_peer_id_);

  signal_client_->Send(SignalServerConnection::MsgSendOffer, jmessage);
}



//
// HotlineDataChannelObserver implementation.
//

void Conductor::OnControlDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel, bool is_local){
  LOG(INFO) << "Main data channel opened.";
  if (client_mode()) {
    if (is_local) {
      local_control_datachannel_->CreateChannel(remote_address_.ToString(), protocol_);
    }
    else{
    }
  }
}


void Conductor::OnControlDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel, bool is_local){
  LOG(INFO) << "Main data channel cloed.";
  if (server_mode()) {
    if (is_local){
      socket_client_.Disconnect();
    }
  }
  else {
    if (is_local) {
      socket_listen_server_.StopListening();
    }
  }
}


void Conductor::OnSocketDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel) {
  if (server_mode()){
    CreateConnectionLane(channel);
  }
}

void Conductor::OnSocketDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel) {
  SocketConnection* socket = channel->DetachSocket();
}

void Conductor::OnCreateChannel(rtc::SocketAddress& remote_address, cricket::ProtocolType protocol){
  ASSERT(server_mode_);
  channel_.Set(remote_address, protocol);
}


void Conductor::OnStopChannel(std::string& channel_name) {
  LaneMessageData* msgdata = NULL;
  rtc::scoped_refptr<HotlineDataChannel> channel = datachannels_[channel_name];
  if (channel==NULL) return;

  channel->closed_by_remote(true);
  msgdata = new LaneMessageData(NULL, channel);
  signal_thread_->Post(this, MsgStopLane, msgdata);
}



void Conductor::OnChannelCreated() {
  ASSERT(!server_mode_);
  socket_listen_server_.Listen(local_address_, protocol_);
}

void Conductor::OnServerSideReady(std::string& channel_name) {
  ASSERT(!server_mode_);

  rtc::scoped_refptr<HotlineDataChannel> channel = datachannels_[channel_name];
  if (channel) {
    channel->SetSocketReady();
    channel->SocketReadEvent();
  }
}

//
// SocketServerObserver implementation.
//

void Conductor::OnSocketOpen(SocketConnection* socket){
  if (!client_mode()){
    CreateConnectionLane(socket);
  }
}
  
void Conductor::OnSocketClosed(SocketConnection* socket){
  socket->DetachChannel();
}

void Conductor::OnSocketStop(SocketConnection* socket) {
  LaneMessageData* msgdata = NULL;
  msgdata = new LaneMessageData(socket, NULL);
  signal_thread_->Post(this, MsgStopLane, msgdata);
}

void Conductor::ConnectToPeer() {

  //: Temporary
  if (server_mode()) return;


  if (peer_connection_.get()) {
    printf("Error: We only support connecting to one peer at a time\n");
    return;
  }

  if (InitializePeerConnection()) {
    peer_connection_->CreateOffer(this, NULL);
  } else {
    printf("Error: Failed to initialize PeerConnection\n");
  }
}


bool Conductor::AddControlDataChannel() {

  typedef std::pair<std::string,
    rtc::scoped_refptr<HotlineDataChannel> > DataChannelObserverPair;

  int current_serial = local_datachannel_serial_++;

  webrtc::DataChannelInit config;
  config.reliable = true;
  config.ordered = true;
  config.id =current_serial;

  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel =
    peer_connection_->CreateDataChannel(kControlDataLabel, &config);

  if (data_channel.get() == NULL) {
    LOG(LS_ERROR) << "CreateDataChannel(" + std::string(kControlDataLabel) +") to PeerConnection failed";
    return false;
  }

  local_control_datachannel_ = new rtc::RefCountedObject<HotlineControlDataChannel>(data_channel, true);
  local_control_datachannel_->RegisterObserver(this);
  return true;
}


bool Conductor::AddPacketDataChannel(std::string* channel_name) {

  typedef std::pair<std::string,
    rtc::scoped_refptr<HotlineDataChannel> > DataChannelObserverPair;

  int current_serial = local_datachannel_serial_++;

  webrtc::DataChannelInit config;
  config.reliable = true;
  config.ordered = true;
  config.id =current_serial;

  *channel_name = std::to_string(current_serial);

  rtc::scoped_refptr<HotlineDataChannel> data_channel_observer;
  if (datachannels_.find(*channel_name) != datachannels_.end()) {
    return true;  // Already added.
  }

  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel =
    peer_connection_->CreateDataChannel(*channel_name, &config);

  if (data_channel.get() == NULL) {
    LOG(LS_ERROR) << "CreateDataChannel to PeerConnection failed";
    return false;
  }

  data_channel_observer = 
                new rtc::RefCountedObject<HotlineDataChannel>(data_channel, true);

  ASSERT(data_channel_observer.get()!=NULL);

  datachannels_.insert(DataChannelObserverPair(data_channel->label(),
                              data_channel_observer));

  data_channel_observer->RegisterObserver(this);
  return true;
}


// create client socket + data channel + server socket connection
bool Conductor::CreateConnectionLane(SocketConnection* connection) {
  std::string channel_name;
  if (!AddPacketDataChannel(&channel_name)) return false;

  rtc::scoped_refptr<HotlineDataChannel> channel = datachannels_[channel_name];
  if (channel==NULL) return false;

  channel->AttachSocket(connection);
  connection->AttachChannel(channel);

  return true;
}

bool Conductor::CreateConnectionLane(rtc::scoped_refptr<HotlineDataChannel> channel) {
  if (channel==NULL) return false;

  SocketConnection* connection = socket_client_.Connect(channel_.remote_address(), channel_.protocol());
  if (connection==NULL) {
    channel->Stop();
    return false;
  }

  channel->AttachSocket(connection);
  connection->AttachChannel(channel);
  connection->SetReady();
  local_control_datachannel_->ServerSideReady(channel->label());

  return true;
}

// delete client socket + data channel + server socket connection
void Conductor::DeleteConnectionLane(SocketConnection* connection, rtc::scoped_refptr<HotlineDataChannel> channel) {
  // Get variables
  if (connection==NULL && channel==NULL) return;

  if (channel == NULL) {
    channel = connection->GetAttachedChannel();
    if (channel==NULL) return;
  }

  if (connection == NULL) {
    connection = channel->GetAttachedSocket();
    if (connection == NULL) return;
  }

  std::string channel_name = channel->label();

  // Delete socket
  if (connection) {
    connection->Close();
  }

  // TODO: Remote comment if datachannel bug on webrtc resolved.
  // Send messagt to remote peer that delete socket.
  if (!channel->closed_by_remote()) {
    local_control_datachannel_->DeleteRemoteChannel(channel_name);
  }

  // Delete datachannel
  if (channel) {
  //:  channel->Close();
  //:  datachannels_.erase(channel_name);
  }

}


void Conductor::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  peer_connection_->SetLocalDescription(
      HotlineSetSessionDescriptionObserver::Create(), desc);

  std::string sdp;
  desc->ToString(&sdp);

  // For loopback test. To save some connecting delay.
  if (loopback_) {
    // Replace message type from "offer" to "answer"
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription("answer", sdp));
    peer_connection_->SetRemoteDescription(
        HotlineSetSessionDescriptionObserver::Create(), session_description);
    return;
  }

  Json::Value jmessage;
  jmessage[kSessionDescriptionTypeName] = desc->type();
  jmessage[kSessionDescriptionSdpName] = sdp;
  jmessage["room_id"] = room_id_;
  jmessage["peer_id"] = std::to_string(remote_peer_id_);

  signal_client_->Send(SignalServerConnection::MsgSendOffer, jmessage);
}

void Conductor::OnFailure(const std::string& error) {
    LOG(LERROR) << error;
}

void Conductor::OnMessage(rtc::Message* msg) {
  try {
    if (msg->message_id == ThreadMsgId::MsgStopLane) {
      LaneMessageData *msgData = static_cast<LaneMessageData*>(msg->pdata);
      if (msgData) {
        DeleteConnectionLane(msgData->socket_connection(), msgData->data_channel());
        delete msgData;
      }
    }
  }
  catch (...) {
    LOG(LS_WARNING) << "Conductor::OnMessage() Exception.";
  }

}


void Conductor::OnReceivedOffer(Json::Value& data) {

  std::string peer_id;
  std::string room_id;
  std::string type;
  std::string sdp;
  uint64 npeer_id;

  if(!rtc::GetStringFromJsonObject(data, "peer_id", &peer_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    printf("Error: Server response error\n");
    return;
  }

  npeer_id = strtoull(peer_id.c_str(), NULL, 10);

  if (!peer_connection_.get()) {
    if (!InitializePeerConnection()) {
      LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
//:      client_->SignOut();
      return;
    }
  } else if (npeer_id != remote_peer_id_) {
    LOG(WARNING) << "Received a message from unknown peer while already in a "
                    "conversation with a different peer.";
    return;
  }

  rtc::GetStringFromJsonObject(data, kSessionDescriptionTypeName, &type);
  if (!type.empty()) {
    if (type == "offer-loopback") {
      // This is a loopback call.
      // Recreate the peerconnection with DTLS disabled.
      if (!ReinitializePeerConnectionForLoopback()) {
        LOG(LS_ERROR) << "Failed to initialize our PeerConnection instance";
        DeletePeerConnection();
//:        client_->SignOut();
      }
      return;
    }

    std::string sdp;
    if (!rtc::GetStringFromJsonObject(data, kSessionDescriptionSdpName, &sdp)) {
      LOG(WARNING) << "Can't parse received session description message.";
      return;
    }
    webrtc::SessionDescriptionInterface* session_description(
        webrtc::CreateSessionDescription(type, sdp));
    if (!session_description) {
      LOG(WARNING) << "Can't parse received session description message.";
      return;
    }

    peer_connection_->SetRemoteDescription(
        HotlineSetSessionDescriptionObserver::Create(), session_description);

    if (session_description->type() ==
        webrtc::SessionDescriptionInterface::kOffer) {
      peer_connection_->CreateAnswer(this, NULL);
    }
    return;
  } else {
    std::string sdp_mid;
    int sdp_mlineindex = 0;
    std::string sdp;
    if (!rtc::GetStringFromJsonObject(data, kCandidateSdpMidName, &sdp_mid) ||
        !rtc::GetIntFromJsonObject(data, kCandidateSdpMlineIndexName,
                              &sdp_mlineindex) ||
        !rtc::GetStringFromJsonObject(data, kCandidateSdpName, &sdp)) {
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
    return;
  }
}


} // namespace hotline
