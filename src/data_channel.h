#ifndef HOTLINE_TUNNEL_DATA_CHANNEL_H_
#define HOTLINE_TUNNEL_DATA_CHANNEL_H_
#pragma once

#include <list>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/json.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "peer_connection_client.h"
#include "defaults.h"

namespace hotline {

class SocketConnection;
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
  explicit HotlineDataChannel(webrtc::DataChannelInterface* channel, bool is_local)
    : channel_(channel), socket_(NULL), callback_(NULL), received_message_count_(0), is_local_(is_local) {
    channel_->RegisterObserver(this);
    state_ = channel_->state();
  }

  virtual ~HotlineDataChannel() {
    channel_->UnregisterObserver();
    channel_->Close();
  }

  void RegisterObserver(HotlineDataChannelObserver* callback);
  bool AttachSocket(SocketConnection* socket);
  SocketConnection* DetachSocket();

  bool Send(char* buf, size_t len);
  void Close();

  std::string label() { return channel_->label(); }
  bool local(){return is_local_;}

  bool IsOpen() const { return state_ == webrtc::DataChannelInterface::kOpen; }
  size_t received_message_count() const { return received_message_count_; }


protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  SocketConnection* socket_;
  webrtc::DataChannelInterface::DataState state_;
  size_t received_message_count_;
  HotlineDataChannelObserver* callback_;
  bool is_local_;

};

//////////////////////////////////////////////////////////////////////


class HotlineControlDataChannel
  : public HotlineDataChannel {
public:
  enum MSGID {
    SetUserArgument
  };

  class ControlMessage;
  explicit HotlineControlDataChannel(webrtc::DataChannelInterface* channel, bool is_local) 
              : HotlineDataChannel(channel, is_local) {}
  virtual ~HotlineControlDataChannel() {}

  bool SendMessage(MSGID id, std::string& data1, std::string& data2, std::string&data3);

protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

};

} // namespace hotline

#endif  // HOTLINE_TUNNEL_DATA_CHANNEL_H_
