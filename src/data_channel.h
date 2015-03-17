#ifndef HOTLINE_TUNNEL_DATA_CHANNEL_H_
#define HOTLINE_TUNNEL_DATA_CHANNEL_H_
#pragma once

#include <list>

#include "webrtc/base/scoped_ptr.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "peer_connection_client.h"
#include "socket_connection.h"
#include "defaults.h"


// HotlineDataChnnelObserver
// Observe data channel statechange including open, close and message incoming from peer.
// Create instance per data channel because OnStateChange() and OnMessage() has 
// no input webrtc::DataChannelInterface pointer argument.
//
class HotineDataChannelObserver
  : public webrtc::DataChannelObserver,
  public rtc::RefCountInterface {
public:

  explicit HotineDataChannelObserver(webrtc::DataChannelInterface* channel, bool is_main)
    : channel_(channel), received_message_count_(0) {
    channel_->RegisterObserver(this);
    state_ = channel_->state();

    is_main_channel_ = is_main;
  }

  virtual ~HotineDataChannelObserver() {
    channel_->UnregisterObserver();
  }

  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

  bool IsMainChannel() const { return is_main_channel_; }
  bool IsOpen() const { return state_ == webrtc::DataChannelInterface::kOpen; }
  size_t received_message_count() const { return received_message_count_; }

  sigslot::signal0<> SignalOpenEvent;

protected:
  bool is_main_channel_;
  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  webrtc::DataChannelInterface::DataState state_;
  size_t received_message_count_;

};


#endif  // HOTLINE_TUNNEL_DATA_CHANNEL_H_
