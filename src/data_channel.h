#ifndef HOTLINE_TUNNEL_DATA_CHANNEL_H_
#define HOTLINE_TUNNEL_DATA_CHANNEL_H_
#pragma once

#include <list>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "peer_connection_client.h"
#include "socket_server.h"
#include "defaults.h"

namespace hotline {

class SocketServerConnection;
class HotlineDataChannel;


//////////////////////////////////////////////////////////////////////
  
struct HotlineDataChannelObserver{
  virtual void OnControlDataChannelOpen(bool is_local) = 0;
  virtual void OnControlDataChannelClosed(bool is_local) = 0;

  virtual void OnSocketDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel) = 0;
  virtual void OnSocketDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel) = 0;

protected:
  virtual ~HotlineDataChannelObserver() {}
};


//////////////////////////////////////////////////////////////////////
// HotlineDataChnnelObserver
// Observe data channel statechange including open, close and message incoming from peer.
// Create instance per data channel because OnStateChange() and OnMessage() has 
// no input webrtc::DataChannelInterface pointer argument.
//
class HotlineDataChannel
  : public webrtc::DataChannelObserver,
    public rtc::RefCountInterface {

public:
  explicit HotlineDataChannel(webrtc::DataChannelInterface* channel)
    : channel_(channel), socket_(NULL), callback_(NULL), received_message_count_(0) {
    channel_->RegisterObserver(this);
    state_ = channel_->state();
  }

  virtual ~HotlineDataChannel() {
    channel_->UnregisterObserver();
    channel_->Close();
  }

  void RegisterObserver(HotlineDataChannelObserver* callback);
  bool AttachSocket(SocketServerConnection* socket);
  SocketServerConnection* DetachSocket();

  void Close();

  std::string label() { return channel_->label(); }
  bool IsOpen() const { return state_ == webrtc::DataChannelInterface::kOpen; }
  size_t received_message_count() const { return received_message_count_; }


protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);
  virtual void HandleChannelClosed();

  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  SocketServerConnection* socket_;
  webrtc::DataChannelInterface::DataState state_;
  size_t received_message_count_;
  HotlineDataChannelObserver* callback_;

};

//////////////////////////////////////////////////////////////////////

class HotlineControlDataChannel
  : public HotlineDataChannel {
public:
  explicit HotlineControlDataChannel(webrtc::DataChannelInterface* channel, bool is_local) 
              : HotlineDataChannel(channel), is_local_(is_local) {}
  virtual ~HotlineControlDataChannel() {}

  bool local(){return is_local_;}

protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

  bool is_local_;
};

} // namespace hotline

#endif  // HOTLINE_TUNNEL_DATA_CHANNEL_H_
