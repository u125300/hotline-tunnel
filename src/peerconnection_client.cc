#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/json.h"
#include "webrtc/base/thread.h"
#include "peerconnection_client.h"

#define HOTLINE_API_VERSION 100    // 1.00


namespace hotline {


PeerConnectionClient::PeerConnectionClient(rtc::Thread* signal_thread)
  : callback_(NULL)
  , signal_thread_(signal_thread)
{
}

PeerConnectionClient::~PeerConnectionClient() {
  
}

bool PeerConnectionClient::InitClientMode(std::string& password, std::string& room_id) {

  if (room_id.length() == 0) {
    printf("Error: please set room id with -r option.\n");
    return false;
  }

  server_mode_ = false;
  password_ = password;
  room_id_ = room_id;

  return true;
}


bool PeerConnectionClient::InitServerMode(std::string& password) {
  server_mode_ = true;
  password_ = password;

  return true;
}


void PeerConnectionClient::RegisterObserver(PeerConnectionClientObserver* callback) {
  ASSERT(callback_ == NULL);
  callback_ = callback;
}


void PeerConnectionClient::Connect(const std::string& url) {
  ws_ = rtc::scoped_ptr<WebSocket>(new WebSocket());
  if (ws_==NULL) {
    LOG(LS_ERROR) << "WebSocket creation failed. ";
    return;
  }

  InitSocketSignals();

  if (!ws_->init(url)) {
    LOG(LS_ERROR) << "WebSocket init failed. ";
    return;
  }

  return;
}


void PeerConnectionClient::InitSocketSignals() {
  ASSERT(ws_.get() != NULL);

  ws_->SignalConnectEvent.connect(this, &PeerConnectionClient::onOpen);
  ws_->SignalCloseEvent.connect(this, &PeerConnectionClient::onClose);
  ws_->SignalErrorEvent.connect(this, &PeerConnectionClient::onError);
  ws_->SignalReadEvent.connect(this, &PeerConnectionClient::onMessage);
}


void PeerConnectionClient::OnMessage(rtc::Message* msg) {

  // Default message id
  if (msg->message_id == 0) {
    ASSERT(callback_ != NULL);
    callback_->OnServerConnectionFailure();
  }
}


void PeerConnectionClient::onOpen(WebSocket* ws) {
  
  if (server_mode_) {
    StartServerMode();
  }
  else {
    StartClientMode();
  }
}

void PeerConnectionClient::onMessage(WebSocket* ws, const WebSocket::Data& data) {
  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(data.bytes, jmessage)) {
    LOG(WARNING) << "Received unknown message. " << data.bytes;
    return;
  }

  MsgID msgid;
  Json::Value payload_data;

  if(!GetIntFromJsonObject(jmessage, "msgid", (int*)&msgid)) return;
  if(!GetValueFromJsonObject(jmessage, "data", &payload_data)) return;

  if (msgid == SigninAsServer) {
    SignedInAsServer(payload_data);
  }
  else if (msgid == SigninAsClient) {
    SignedInAsClient(payload_data);
  }
}

void PeerConnectionClient::onClose(WebSocket* ws) {
  
}


void PeerConnectionClient::onError(WebSocket* ws, const WebSocket::ErrorCode& error) {
  
}


bool PeerConnectionClient::StartClientMode() {

  Json::Value jdata;
  jdata["password"] = password_;
  jdata["room_id"] = room_id_;

  return Send(SigninAsClient, jdata);
}

bool PeerConnectionClient::StartServerMode() {

  Json::Value jdata;
  jdata["password"] = password_;

  return Send(SigninAsServer, jdata);
}


void PeerConnectionClient::SignedInAsServer(Json::Value& data) {
  bool successful;
  std::string room_id;
  std::string peer_id;
  uint64 int64_peer_id;

  if(!GetBoolFromJsonObject(data, "successful", &successful)
      || !GetStringFromJsonObject(data, "room_id", &room_id)
      || !GetStringFromJsonObject(data, "peer_id", &peer_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    printf("Error: Server response error\n");
    Exit();
    return;
  }

  if (!successful) {
    LOG(LS_WARNING) << "Room creation failed.";
    Exit();
    return;
  }

  room_id_ = room_id;
  int64_peer_id = strtoull(peer_id.c_str(), NULL, 10);
  callback_->OnSignedInAsServer(room_id, int64_peer_id);
}

void PeerConnectionClient::SignedInAsClient(Json::Value& data) {
  bool successful;
  std::string room_id;
  std::string peer_id;
  uint64 int64_peer_id;

  if(!GetBoolFromJsonObject(data, "successful", &successful)
      || !GetStringFromJsonObject(data, "room_id", &room_id)
      || !GetStringFromJsonObject(data, "peer_id", &peer_id)){
    LOG(LS_WARNING) << "Invalid message format";
    Exit();
    return;
  }

  if (!successful) {
    std::string message;
    if (GetStringFromJsonObject(data, "message", &message)) {
      printf("Error: %s\n", message.c_str());
    }

    Exit();
    return;
  }

  ASSERT(room_id == room_id_);
  int64_peer_id = strtoull(peer_id.c_str(), NULL, 10);
}


void PeerConnectionClient::Exit() {
  ASSERT(signal_thread_ != NULL);
  signal_thread_->Post(this);
}

template<typename T>
bool PeerConnectionClient::Send(const MsgID msgid, T& data) {

  int version = HOTLINE_API_VERSION;

  Json::FastWriter writer;
  Json::Value jmessage;

  jmessage["msgid"] = msgid;
  jmessage["ver"] = version;
  jmessage["data"] = data;

  if (ws_->getReadyState() != WebSocket::State::OPEN) {
    LOG(LS_ERROR) << "WebSocket not opened, Send("
        + std::to_string(msgid) + ", " + std::to_string(version) + ", " + writer.write(data) + ") failed.";
    return false;
  }

  ws_->send(writer.write(jmessage));
  return true;
}


///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

