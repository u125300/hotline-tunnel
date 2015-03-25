#include "htn_config.h"

#include "webrtc/base//common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"
#include "data_channel.h"

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
  }
  else if (state_ == webrtc::DataChannelInterface::kClosed) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been closed.";
    HandleChannelClosed();
  }
  else if (state_ == webrtc::DataChannelInterface::kConnecting) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is connecting.";
  }
  else if (state_ == webrtc::DataChannelInterface::kClosing) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is closing.";
  }
}


bool HotlineDataChannel::AttachSocket(SocketServerConnection *socket) {
  if (socket_!=NULL) return false;
  socket_ = socket;
  return true;
}
  
SocketServerConnection* HotlineDataChannel::DetachSocket() {
  SocketServerConnection* socket = socket_;
  socket_ = NULL;
  return socket;
}

void HotlineDataChannel::Close() {
  channel_->Close();
}


void HotlineDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
  ++received_message_count_;
  LOG(INFO) << __FUNCTION__ << " " << " received_message_count_ = " << received_message_count_;
}

void HotlineDataChannel::HandleChannelClosed() {

  if (callback_) {
    callback_->OnSocketDataChannelClosed(this);
  }
}


void HotlineControlDataChannel::OnStateChange() {
  HotlineDataChannel::OnStateChange();

  if (state_ == webrtc::DataChannelInterface::kOpen){
    if (callback_) callback_->OnControlDataChannelOpen(is_local_);
  }
  else if (state_ == webrtc::DataChannelInterface::kClosed){
    if (callback_) callback_->OnControlDataChannelClosed(is_local_);
  }
}

void HotlineControlDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
}

} // namespace hotline