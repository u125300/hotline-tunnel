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


void SignalServerConnection::RegisterObserver(SignalServerConnectionObserver* callback) {
  ASSERT(callback_ == NULL);
  callback_ = callback;
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
    if (msg->message_id == ThreadMsgId::MsgServer) {
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
      case MsgReceivedOffer:
        OnReceivedOffer(server_msg->data());
        break;
      default:
        break;
      }
    }
    else if (msg->message_id == ThreadMsgId::MsgClose) {
      ASSERT(callback_ != NULL);
      callback_->OnServerConnectionFailure();
    }
  }
  catch (...) {
    LOG(LS_WARNING) << "OnMessage() Exception.";
  }
}


void SignalServerConnection::onOpen(WebSocket* ws) {
  callback_->OnConnected();
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

  if(!GetIntFromJsonObject(jmessage, "msgid", (int*)&msgid)) return;
  if(!GetValueFromJsonObject(jmessage, "data", &payload_data)) return;

  msgdata = new ServerMessageData(msgid, payload_data);
  signal_thread_->Post(this, ThreadMsgId::MsgServer, msgdata);
}

void SignalServerConnection::onClose(WebSocket* ws) {
  
}


void SignalServerConnection::onError(WebSocket* ws, const WebSocket::ErrorCode& error) {
  
}

void SignalServerConnection::OnCreatedRoom(Json::Value& data) {
  bool successful;
  std::string room_id;

  if(!GetBoolFromJsonObject(data, "successful", &successful)
      || !GetStringFromJsonObject(data, "room_id", &room_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    printf("Error: Server response error\n");
    Close();
    return;
  }

  if (!successful) {
    LOG(LS_WARNING) << "Room creation failed.";
    Close();
    return;
  }

  callback_->OnCreatedRoom(room_id);
}

void SignalServerConnection::OnSignedIn(Json::Value& data) {
  bool successful;
  std::string room_id;
  std::string peer_id;
  std::string message;
  uint64 npeer_id;

  if(!GetBoolFromJsonObject(data, "successful", &successful)
      || !GetStringFromJsonObject(data, "room_id", &room_id)
      || !GetStringFromJsonObject(data, "peer_id", &peer_id)
      || !GetStringFromJsonObject(data, "message", &message)
      ) {
    LOG(LS_WARNING) << "Invalid message format";
    printf("Error: Server response error\n");
    Close();
    return;
  }

  if (!successful) {
    LOG(LS_WARNING) << message.c_str();
    Close();
    return;
  }

  npeer_id = strtoull(peer_id.c_str(), NULL, 10);
  callback_->OnSignedIn(room_id, npeer_id);
}


void SignalServerConnection::OnPeerConnected(Json::Value& data) {
  std::string peer_id;
  uint64 npeer_id;

  if(!GetStringFromJsonObject(data, "peer_id", &peer_id)) {
    LOG(LS_WARNING) << "Invalid message format";
    printf("Error: Server response error\n");
    return;
  }

  npeer_id = strtoull(peer_id.c_str(), NULL, 10);
  
  callback_->OnPeerConnected(npeer_id);
}

void SignalServerConnection::OnReceivedOffer(Json::Value& data) {
  callback_->OnReceivedOffer(data);
}


void SignalServerConnection::Close() {
  ASSERT(signal_thread_ != NULL);
  signal_thread_->Post(this);
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

