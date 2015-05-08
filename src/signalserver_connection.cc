#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/json.h"
#include "webrtc/base/thread.h"
#include "signalserver_connection.h"

#define HOTLINE_API_VERSION 100    // 1.00


namespace hotline {


SignalServerConnection::SignalServerConnection(rtc::Thread* signal_thread)
  : callback_(NULL)
  , signal_thread_(signal_thread)
{
}

SignalServerConnection::~SignalServerConnection() {
}


void SignalServerConnection::Connect(const std::string& url) {
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

void SignalServerConnection::CreateRoom(const std::string& password) {

  Json::Value jdata;
  jdata["password"] = password;

  Send(MsgCreateRoom, jdata);
}

void SignalServerConnection::SignIn(std::string& room_id, std::string& password) {
  Json::Value jdata;

  jdata["room_id"] = room_id;
  jdata["password"] = password;

  Send(MsgSignIn, jdata);
}

void SignalServerConnection::SignOut(std::string& room_id) {
  Json::Value jdata;

  jdata["room_id"] = room_id;

  Send(MsgSignOut, jdata);
}

void SignalServerConnection::RegisterObserver(SignalServerConnectionObserver* callback) {
  ASSERT(callback_ == NULL);
  callback_ = callback;
}

void SignalServerConnection::UnregisterObserver(SignalServerConnectionObserver* callback) {
  ASSERT(callback_ != NULL);
  callback_ = NULL;
}



void SignalServerConnection::InitSocketSignals() {
  ASSERT(ws_.get() != NULL);

  ws_->SignalConnectEvent.connect(this, &SignalServerConnection::onOpen);
  ws_->SignalCloseEvent.connect(this, &SignalServerConnection::onClose);
  ws_->SignalErrorEvent.connect(this, &SignalServerConnection::onError);
  ws_->SignalReadEvent.connect(this, &SignalServerConnection::onMessage);
}


void SignalServerConnection::OnMessage(rtc::Message* msg) {

  try {
    if (msg->message_id == ThreadMsgId::MsgServerMessage) {
      rtc::scoped_ptr<ServerMessageData> server_msg(static_cast<ServerMessageData*>(msg->pdata));
    
      switch (server_msg->msgid()) {
      case MsgCreateRoom:
        OnCreatedRoom(server_msg->data());
        break;
      case MsgSignIn:
        OnSignedIn(server_msg->data());
       break;
      case MsgPeerConnected:
        OnPeerConnected(server_msg->data());
        break;
      case MsgPeerDisconnected:
        OnPeerDisconnected(server_msg->data());
        break;
      case MsgReceivedOffer:
        OnReceivedOffer(server_msg->data());
        break;
      default:
        break;
      }
    }
  }
  catch (...) {
    LOG(LS_WARNING) << "SignalServerConnection::OnMessage() Exception.";
  }
}


void SignalServerConnection::onOpen(WebSocket* ws) {
  if (callback_) {
    callback_->OnConnected();
  }
}

void SignalServerConnection::onMessage(WebSocket* ws, const WebSocket::Data& data) {
  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(data.bytes, jmessage)) {
    LOG(WARNING) << "Received unknown message. " << data.bytes;
    return;
  }

  ServerMessageData* msgdata = NULL;
  MsgID msgid;
  Json::Value payload_data;

  if(!rtc::GetIntFromJsonObject(jmessage, "msgid", (int*)&msgid)) return;
  if(!rtc::GetValueFromJsonObject(jmessage, "data", &payload_data)) return;

  msgdata = new ServerMessageData(msgid, payload_data);
  signal_thread_->Post(this, ThreadMsgId::MsgServerMessage, msgdata);
}

void SignalServerConnection::onClose(WebSocket* ws) {
  if (callback_) {
    callback_->OnDisconnected();
  }
}


void SignalServerConnection::onError(WebSocket* ws, const WebSocket::ErrorCode& error) {
  ServerConnectionFailure();
}

void SignalServerConnection::OnCreatedRoom(Json::Value& data) {
  bool successful;
  std::string room_id;

  if(!rtc::GetBoolFromJsonObject(data, "successful", &successful)
      || !rtc::GetStringFromJsonObject(data, "room_id", &room_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    ServerConnectionFailure();
    return;
  }

  if (!successful) {
    LOG(LS_WARNING) << "Room creation failed.";
    ServerConnectionFailure();
    return;
  }

  if (callback_) {
    callback_->OnCreatedRoom(room_id);
  }
}

void SignalServerConnection::OnSignedIn(Json::Value& data) {
  bool successful;
  std::string room_id;
  std::string peer_id;
  std::string message;
  uint64 npeer_id;

  if(!rtc::GetBoolFromJsonObject(data, "successful", &successful)
      || !rtc::GetStringFromJsonObject(data, "room_id", &room_id)
      || !rtc::GetStringFromJsonObject(data, "peer_id", &peer_id)
      || !rtc::GetStringFromJsonObject(data, "message", &message)
      ) {
    LOG(LS_WARNING) << "Invalid message format";
    ServerConnectionFailure();
    return;
  }

  if (!successful) {
    LOG(LS_WARNING) << message.c_str();
    ServerConnectionFailure();
    return;
  }

  npeer_id = strtoull(peer_id.c_str(), NULL, 10);
  if (callback_) {
    callback_->OnSignedIn(room_id, npeer_id);
  }
}


void SignalServerConnection::OnPeerConnected(Json::Value& data) {
  std::string peer_id;
  uint64 npeer_id;

  if(!rtc::GetStringFromJsonObject(data, "peer_id", &peer_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    return;
  }

  npeer_id = strtoull(peer_id.c_str(), NULL, 10);
  
  if (callback_) {
    callback_->OnPeerConnected(npeer_id);
  }
}

void SignalServerConnection::OnPeerDisconnected(Json::Value& data) {
  std::string peer_id;
  uint64 npeer_id;

  if(!rtc::GetStringFromJsonObject(data, "peer_id", &peer_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    return;
  }

  npeer_id = strtoull(peer_id.c_str(), NULL, 10);
  
  if (callback_) {
    callback_->OnPeerDisconnected(npeer_id);
  }
}

void SignalServerConnection::OnReceivedOffer(Json::Value& data) {
  if (callback_) {
    callback_->OnReceivedOffer(data);
  }
}

void SignalServerConnection::ServerConnectionFailure() {
  if (callback_) {
    callback_->OnServerConnectionFailure();
  }
}

template<typename T>
bool SignalServerConnection::Send(const MsgID msgid, T& data) {

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

bool SignalServerConnection::Send(const MsgID msgid) {
  Json::Value jmessage;
  return Send(msgid, jmessage);
}


///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

