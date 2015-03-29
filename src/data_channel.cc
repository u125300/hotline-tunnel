#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/logging.h"
#include "data_channel.h"
#include "socket.h"


#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif


namespace hotline {

void HotlineDataChannel::RegisterObserver(HotlineDataChannelObserver* callback) {
  callback_ = callback;
}

void HotlineDataChannel::OnStateChange() {
  state_ = channel_->state();

  if (state_ == webrtc::DataChannelInterface::kOpen){
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been openned.";
    if (callback_) callback_->OnSocketDataChannelOpen(this);
  }
  else if (state_ == webrtc::DataChannelInterface::kClosed) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been closed.";
    if (callback_)  callback_->OnSocketDataChannelClosed(this);
  }
  else if (state_ == webrtc::DataChannelInterface::kConnecting) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is connecting.";
  }
  else if (state_ == webrtc::DataChannelInterface::kClosing) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is closing.";
  }
}


bool HotlineDataChannel::AttachSocket(SocketConnection *socket) {
  if (socket_!=NULL) return false;
  socket_ = socket;
  return true;
}
  
SocketConnection* HotlineDataChannel::DetachSocket() {
  SocketConnection* socket = socket_;
  socket_ = NULL;
  return socket;
}

bool HotlineDataChannel::Send(char* buf, size_t len) {

  if (channel_==NULL || channel_->state()!=webrtc::DataChannelInterface::kOpen) return false;

  rtc::Buffer buffer(buf, len);
  bool result = channel_->Send(webrtc::DataBuffer(buffer, true));
  ASSERT(result);

  return result;
}

void HotlineDataChannel::Close() {
    channel_->Close();
}


void HotlineDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
  if (socket_) socket_->Send(buffer);
}


void HotlineControlDataChannel::OnStateChange() {

  state_ = channel_->state();

  if (state_ == webrtc::DataChannelInterface::kOpen){
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been openned.";
    if (callback_) callback_->OnControlDataChannelOpen(this, local());

  }
  else if (state_ == webrtc::DataChannelInterface::kClosed) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been closed.";
    if (callback_) callback_->OnControlDataChannelClosed(this, local());
  }
  else if (state_ == webrtc::DataChannelInterface::kConnecting) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is connecting.";
  }
  else if (state_ == webrtc::DataChannelInterface::kClosing) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is closing.";
  }
}

void HotlineControlDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
  Json::Reader reader;
  Json::Value jmessage;
  if (!reader.parse(buffer.data.data(), jmessage)) {
    LOG(WARNING) << "Received unknown message. " << buffer.data.data();
    return;
  }

  MSGID id;
  Json::Value data;

  if(!GetIntFromJsonObject(jmessage, "id", (int*)&id)) return;
  if (!GetValueFromJsonObject(jmessage, "data", &data)) return;

  switch (id) {
  case MsgCreateClient:
    OnCreateClient(data);
    break;

  case MsgClientCreated:
    OnClientCreated(data);
    break;
  }
}



bool HotlineControlDataChannel::CreateClient(uint64 client_id, std::string& remote_address, cricket::ProtocolType protocol) {
  Json::StyledWriter writer;
  Json::Value jmessage;
  Json::Value data;

  data["id"] = std::to_string(client_id);
  data["remote_address"] = remote_address;
  data["protocol"] = protocol;


  jmessage["id"] = MSGID::MsgCreateClient;
  jmessage["data"] = data;

  webrtc::DataBuffer buffer(writer.write(jmessage));
  return channel_->Send(buffer);
}

void HotlineControlDataChannel::OnCreateClient(Json::Value& json_data) {

  std::string id_string;
  std::string remote_address_string;
  cricket::ProtocolType protocol;

  if (!GetStringFromJsonObject(json_data, "id", &id_string)) return;
  if (!GetStringFromJsonObject(json_data, "remote_address", &remote_address_string)) return;
  if (!GetIntFromJsonObject(json_data, "protocol", (int*)&protocol)) return;

  uint64 id = std::stoull(id_string);
  rtc::SocketAddress remote_address;
  if (!remote_address.FromString(remote_address_string)) return;
  if (protocol != cricket::PROTO_UDP && protocol != cricket::PROTO_TCP) return;

  callback_->OnCreateClient(id, remote_address, protocol);
  ClientCreated(id);

  return;
}

bool HotlineControlDataChannel::ClientCreated(uint64 client_id) {
  Json::StyledWriter writer;
  Json::Value jmessage;
  Json::Value data;

  data["id"] = std::to_string(client_id);


  jmessage["id"] = MSGID::MsgClientCreated;
  jmessage["data"] = data;

  webrtc::DataBuffer buffer(writer.write(jmessage));
  return channel_->Send(buffer);
}


void HotlineControlDataChannel::OnClientCreated(Json::Value& json_data) {

  std::string id_string;

  if (!GetStringFromJsonObject(json_data, "id", &id_string)) return;

  uint64 id = std::stoull(id_string);

  callback_->OnClientCreated(id);  
  return;
}
} // namespace hotline