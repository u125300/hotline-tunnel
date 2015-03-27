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
  socket_->Send(buffer);
}


bool HotlineControlDataChannel::SendMessage(MSGID id,
                                            std::string& data1,
                                            std::string& data2,
                                            std::string&data3) {
  Json::StyledWriter writer;
  Json::Value jmessage;
  jmessage["id"] = id;
  jmessage["data1"] = data1;
  jmessage["data2"] = data2;
  jmessage["data3"] = data3;

  webrtc::DataBuffer buffer(writer.write(jmessage));
  return channel_->Send(buffer);
}


void HotlineControlDataChannel::OnStateChange() {

  state_ = channel_->state();

  if (state_ == webrtc::DataChannelInterface::kOpen){
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been openned.";
    if (callback_) callback_->OnControlDataChannelOpen(local());
  }
  else if (state_ == webrtc::DataChannelInterface::kClosed) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been closed.";
    if (callback_) callback_->OnControlDataChannelClosed(local());
  }
  else if (state_ == webrtc::DataChannelInterface::kConnecting) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is connecting.";
  }
  else if (state_ == webrtc::DataChannelInterface::kClosing) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is closing.";
  }
}

void HotlineControlDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
}



} // namespace hotline