#ifndef HOTLINE_TUNNEL_DATA_CHANNEL_H_
#define HOTLINE_TUNNEL_DATA_CHANNEL_H_
#pragma once

#include <list>

#include "webrtc/base/scoped_ptr.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "peer_connection_client.h"
#include "socket_server.h"
#include "defaults.h"

namespace hotline {


//////////////////////////////////////////////////////////////////////
  
struct ControlDataChannelObserver{
  virtual void OnControlDataChannelOpen(bool is_local) = 0;
  virtual void OnControlDataChannelClosed(bool is_local) = 0;

protected:
  virtual ~ControlDataChannelObserver() {}
};


//////////////////////////////////////////////////////////////////////
// HotlineDataChnnelObserver
// Observe data channel statechange including open, close and message incoming from peer.
// Create instance per data channel because OnStateChange() and OnMessage() has 
// no input webrtc::DataChannelInterface pointer argument.
//
class HotineDataChannel
  : public webrtc::DataChannelObserver,
  public rtc::RefCountInterface {
public:

  explicit HotineDataChannel(webrtc::DataChannelInterface* channel)
    : channel_(channel), received_message_count_(0) {
    channel_->RegisterObserver(this);
    state_ = channel_->state();
  }

  virtual ~HotineDataChannel() {
    channel_->UnregisterObserver();
  }

  std::string label() { return channel_->label(); }
  bool IsOpen() const { return state_ == webrtc::DataChannelInterface::kOpen; }
  size_t received_message_count() const { return received_message_count_; }

protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  webrtc::DataChannelInterface::DataState state_;
  size_t received_message_count_;

};

//////////////////////////////////////////////////////////////////////

class HotlineControlDataChannel
  : public HotineDataChannel {
public:
  explicit HotlineControlDataChannel(webrtc::DataChannelInterface* channel, bool is_local) 
              : HotineDataChannel(channel), is_local_(is_local) {}
  virtual ~HotlineControlDataChannel() {}

  void RegisterObserver(ControlDataChannelObserver* callback);
  bool local(){return is_local_;}

protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

  bool is_local_;
  ControlDataChannelObserver* callback_;
};

} // namespace hotline

#endif  // HOTLINE_TUNNEL_DATA_CHANNEL_H_
