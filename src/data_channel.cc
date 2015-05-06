#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/json.h"
#include "data_channel.h"
#include "socket.h"


#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif


namespace hotline {

HotlineDataChannel::HotlineDataChannel(webrtc::DataChannelInterface* channel, bool is_local)
  : channel_(channel), socket_(NULL), callback_(NULL), is_local_(is_local), is_control_channel_(false), closed_by_remote_(false) {
  channel_->RegisterObserver(this);
  state_ = channel_->state();
}

HotlineDataChannel::~HotlineDataChannel() {
  channel_->UnregisterObserver();
  channel_->Close();
}

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

SocketConnection* HotlineDataChannel::GetAttachedSocket() {
  return socket_;
}

void HotlineDataChannel::SetSocketReady() {
  if (socket_) {
    socket_->SetReady();
  }
}

void HotlineDataChannel::SocketReadEvent() {
  if (socket_) {
    socket_->ReadEvent();
  }
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

void HotlineDataChannel::Stop() {
  callback_->OnStopChannel(label());
}


void HotlineDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
  if (socket_) {
    bool send_result = socket_->Send(buffer);
    ASSERT(send_result);
  }
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

  if(!rtc::GetIntFromJsonObject(jmessage, "id", (int*)&id)) return;
  if (!rtc::GetValueFromJsonObject(jmessage, "data", &data)) return;

  switch (id) {
  case MsgCreateChannel:
    OnCreateChannel(data);
    break;

  case MsgChannelCreated:
    OnChannelCreated(data);
    break;

  case MsgDeleteChannel:
    OnDeleteRemoteChannel(data);
    break;

  case MsgServerSideReady:
    OnServerSideReady(data);
    break;

  default:
    break;
  }
}



bool HotlineControlDataChannel::CreateChannel(std::string& remote_address, cricket::ProtocolType protocol) {
  Json::FastWriter writer;
  Json::Value jmessage;
  Json::Value data;

  data["remote_address"] = remote_address;
  data["protocol"] = protocol;

  jmessage["id"] = MsgCreateChannel;
  jmessage["data"] = data;

  webrtc::DataBuffer buffer(writer.write(jmessage));
  return channel_->Send(buffer);
}


void HotlineControlDataChannel::OnCreateChannel(Json::Value& json_data) {
  std::string remote_address_string;
  cricket::ProtocolType protocol;

  if (!rtc::GetStringFromJsonObject(json_data, "remote_address", &remote_address_string)) return;
  if (!rtc::GetIntFromJsonObject(json_data, "protocol", (int*)&protocol)) return;

  rtc::SocketAddress remote_address;
  if (!remote_address.FromString(remote_address_string)) return;
  if (protocol != cricket::PROTO_UDP && protocol != cricket::PROTO_TCP) return;

  callback_->OnCreateChannel(remote_address, protocol);
  ChannelCreated();

  return;
}

bool HotlineControlDataChannel::ChannelCreated() {
  Json::FastWriter writer;
  Json::Value jmessage;
  Json::Value data;

  jmessage["id"] = MsgChannelCreated;
  jmessage["data"] = data;

  webrtc::DataBuffer buffer(writer.write(jmessage));
  return channel_->Send(buffer);
}


void HotlineControlDataChannel::OnChannelCreated(Json::Value& json_data) {
  callback_->OnChannelCreated();  
  return;
}

bool HotlineControlDataChannel::ServerSideReady(std::string& channel_name) {
  Json::FastWriter writer;
  Json::Value jmessage;
  Json::Value data;

  data["channel_name"] = channel_name;


  jmessage["id"] = MsgServerSideReady;
  jmessage["data"] = data;

  webrtc::DataBuffer buffer(writer.write(jmessage));
  return channel_->Send(buffer);
}


void HotlineControlDataChannel::OnServerSideReady(Json::Value& json_data) {
  std::string channel_name;
  if (!rtc::GetStringFromJsonObject(json_data, "channel_name", &channel_name)) return;
  callback_->OnServerSideReady(channel_name);  
}


bool HotlineControlDataChannel::DeleteRemoteChannel(std::string& channel_name) {
  Json::FastWriter writer;
  Json::Value jmessage;
  Json::Value data;

  data["channel_name"] = channel_name;


  jmessage["id"] = MsgDeleteChannel;
  jmessage["data"] = data;

  webrtc::DataBuffer buffer(writer.write(jmessage));
  return channel_->Send(buffer);
}

void HotlineControlDataChannel::OnDeleteRemoteChannel(Json::Value& json_data) {
  std::string channel_name;
  if (!rtc::GetStringFromJsonObject(json_data, "channel_name", &channel_name)) return;
  callback_->OnStopChannel(channel_name);  
}

} // namespace hotline