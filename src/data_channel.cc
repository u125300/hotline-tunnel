#include "htn_config.h"

#include "webrtc/base//common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"
#include "data_channel.h"

#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif


namespace hotline {

void HotineDataChannel::OnStateChange() {
  state_ = channel_->state();

  if (state_ == webrtc::DataChannelInterface::kOpen){
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been openned.";
  }
  else if (state_ == webrtc::DataChannelInterface::kClosed) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel has been closed.";
  }
  else if (state_ == webrtc::DataChannelInterface::kConnecting) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is connecting.";
  }
  else if (state_ == webrtc::DataChannelInterface::kClosing) {
    LOG(INFO) << __FUNCTION__ << " " << " data channel is closing.";
  }
}


void HotineDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
  ++received_message_count_;
  LOG(INFO) << __FUNCTION__ << " " << " received_message_count_ = " << received_message_count_;
}




void HotlineControlDataChannel::RegisterObserver(ControlDataChannelObserver* callback) {
  callback_ = callback;
}

void HotlineControlDataChannel::OnStateChange() {
  HotineDataChannel::OnStateChange();

  if (state_ == webrtc::DataChannelInterface::kOpen){
    callback_->OnControlDataChannelOpen(is_local_);
  }
  else if (state_ == webrtc::DataChannelInterface::kClosed){
    callback_->OnControlDataChannelClosed(is_local_);
  }
}

void HotlineControlDataChannel::OnMessage(const webrtc::DataBuffer& buffer) {
}

} // namespace hotline