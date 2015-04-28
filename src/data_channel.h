#ifndef HOTLINE_TUNNEL_DATA_CHANNEL_H_
#define HOTLINE_TUNNEL_DATA_CHANNEL_H_
#pragma once

#include <list>

#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/scoped_ref_ptr.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/p2p/base/portinterface.h"
#include "webrtc/base/json.h"
#include "talk/app/webrtc/mediastreaminterface.h"
#include "talk/app/webrtc/peerconnectioninterface.h"
#include "defaults.h"

namespace hotline {

class SocketConnection;
class HotlineDataChannel;


//////////////////////////////////////////////////////////////////////
  
struct HotlineDataChannelObserver{
  virtual void OnControlDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel, bool is_local) = 0;
  virtual void OnControlDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel, bool is_local) = 0;
  virtual void OnSocketDataChannelOpen(rtc::scoped_refptr<HotlineDataChannel> channel) = 0;
  virtual void OnSocketDataChannelClosed(rtc::scoped_refptr<HotlineDataChannel> channel) = 0;

  virtual void OnCreateChannel(rtc::SocketAddress& remote_address, cricket::ProtocolType protocol) = 0;
  virtual void OnChannelCreated() = 0;
  virtual void OnServerSideReady(std::string& channel_name) = 0;

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
    : channel_(channel), socket_(NULL), callback_(NULL), is_local_(is_local), is_control_channel_(false) {
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
  void SetSocketReady();
  void SocketReadEvent();

  bool Send(char* buf, size_t len);
  void Close();

  std::string label() { return channel_->label(); }
  bool local(){return is_local_;}
  bool controlchannel(){return is_control_channel_;}

  bool IsOpen() const { return state_ == webrtc::DataChannelInterface::kOpen; }


protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

  rtc::scoped_refptr<webrtc::DataChannelInterface> channel_;
  SocketConnection* socket_;
  webrtc::DataChannelInterface::DataState state_;
  HotlineDataChannelObserver* callback_;
  bool is_local_;
  bool is_control_channel_;

};

//////////////////////////////////////////////////////////////////////


class HotlineControlDataChannel
  : public HotlineDataChannel {
public:
  enum MSGID {
    MsgCreateChannel,
    MsgChannelCreated,
    MsgServerSideReady
  };

  class ControlMessage;
  explicit HotlineControlDataChannel(webrtc::DataChannelInterface* channel, bool is_local) 
              : HotlineDataChannel(channel, is_local){is_control_channel_ = true;}
  virtual ~HotlineControlDataChannel() {}

  bool CreateChannel(std::string& remote_address, cricket::ProtocolType protocol);
  bool ChannelCreated();
  bool ServerSideReady(std::string& channel_name);

protected:
  virtual void OnStateChange();
  virtual void OnMessage(const webrtc::DataBuffer& buffer);

private:
  void OnCreateChannel(Json::Value& json_data);
  void OnChannelCreated(Json::Value& json_data);
  void OnServerSideReady(Json::Value& json_data);

};

} // namespace hotline

#endif  // HOTLINE_TUNNEL_DATA_CHANNEL_H_
