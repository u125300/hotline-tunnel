#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/json.h"
#include "peerconnection_client.h"

#define HOTLINE_API_VERSION 100    // 1.00


namespace hotline {

PeerConnectionClient::PeerConnectionClient(bool server_mode)
  : server_mode_(server_mode)
  , id_(0)
{
}

PeerConnectionClient::~PeerConnectionClient() {
 
}

void PeerConnectionClient::RegisterObserver(PeerConnectionClientObserver* callback) {

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
  
}


void PeerConnectionClient::onOpen(WebSocket* ws) {
  
  if (server_mode_) {
    StartServerMode();
  }
  else {
  }
}

void PeerConnectionClient::onMessage(WebSocket* ws, const WebSocket::Data& data) {
  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(data.bytes, jmessage)) {
    LOG(WARNING) << "Received unknown message. " << data.bytes;
    return;
  }

  std::string msgid;
  Json::Value payload_data;

  if(!GetStringFromJsonObject(jmessage, "msgid", &msgid)) return;
  if(!GetValueFromJsonObject(jmessage, "data", &payload_data)) return;

  if (msgid == std::string("create")) {
    created(payload_data);
  }
  else if (msgid == std::string("join")) {
    joined(payload_data);
  }

}

void PeerConnectionClient::onClose(WebSocket* ws) {
  
}
  

void PeerConnectionClient::onError(WebSocket* ws, const WebSocket::ErrorCode& error) {
  
}


bool PeerConnectionClient::StartClientMode(int id) {

  Send("join", std::to_string(id));

  return false;
}

bool PeerConnectionClient::StartServerMode() {

  //
  // Make new room
  //

  Send("create", "");

  return true;
}


void PeerConnectionClient::created(Json::Value& data) {
  bool successful;
  int id;

  if(!GetBoolFromJsonObject(data, "successful", &successful)
      || !GetIntFromJsonObject(data, "id", &id)) {
    LOG(LS_WARNING) << "Invalid message format";
    return;
  }

  if (!successful) {
    LOG(LS_WARNING) << "Room creation failed.";
    return;
  }

  id_ = id;
}

void PeerConnectionClient::joined(Json::Value& data) {
  bool successful;
  int id;

  if(!GetBoolFromJsonObject(data, "successful", &successful)
      || !GetIntFromJsonObject(data, "id", &id)){
    LOG(LS_WARNING) << "Invalid message format";
    return;
  }

}

template<typename T>
bool PeerConnectionClient::Send(const std::string msgid, T& data) {

  int version = HOTLINE_API_VERSION;

  Json::StyledWriter writer;
  Json::Value jmessage;

  jmessage["msgid"] = msgid;
  jmessage["ver"] = version;
  jmessage["data"] = data;

  if (ws_->getReadyState() != WebSocket::State::OPEN) {
    LOG(LS_ERROR) << "WebSocket not opened, Send("
        + msgid + ", " + std::to_string(version) + ", " + data + ") failed.";
    return false;
  }

  ws_->send(writer.write(jmessage));
  return true;
}


///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

