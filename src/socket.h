#ifndef HOTLINE_TUNNEL_SOCKET_H_
#define HOTLINE_TUNNEL_SOCKET_H_
#pragma once

#include <list>

#include "webrtc/base/stream.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/socketstream.h"
#include "webrtc/p2p/base/portinterface.h"
#include "webrtc/base/refcount.h"
#include "data_channel.h"


namespace hotline {

class SocketBase;
class HotlineDataChannel;


//////////////////////////////////////////////////////////////////////
  
class SocketObserver{
public:
  virtual void OnSocketOpen(SocketConnection* socket) = 0;
  virtual void OnSocketClosed(SocketConnection* socket) = 0;
};


//////////////////////////////////////////////////////////////////////


class SocketConnection : public sigslot::has_slots<> {
 public:
  enum { kBufferSize = 64 * 1024 };

  SocketConnection(SocketBase* server);
  virtual ~SocketConnection();

  bool AttachChannel(rtc::scoped_refptr<HotlineDataChannel> channel);
  rtc::scoped_refptr<HotlineDataChannel>  DetachChannel();

  void BeginProcess(rtc::StreamInterface* stream);
  rtc::StreamInterface* EndProcess();
  bool Send(const webrtc::DataBuffer& buffer);
  void Close();


 protected:
  void OnStreamEvent(rtc::StreamInterface* stream, int events, int error);
  void HandleStreamClose();

  void DoReceiveLoop();
  void flush_data();

  SocketBase* server_;
  rtc::scoped_refptr<HotlineDataChannel> channel_;
  rtc::StreamInterface* stream_;
  bool closing_;

  char send_buffer_[kBufferSize];
  size_t send_len_;

  char recv_buffer_[kBufferSize];
  size_t recv_len_;
};

//////////////////////////////////////////////////////////////////////

class SocketBase {
public:
  SocketBase();
  virtual ~SocketBase();

  void RegisterObserver(SocketObserver* callback);
  void UnregisterObserver();

  bool HandleConnection(rtc::StreamInterface* stream);
  void Remove(SocketConnection* connection);

  // Due to sigslot issues, we can't destroy some streams at an arbitrary time.
  sigslot::signal3<SocketBase*, SocketConnection*, rtc::StreamInterface*> SignalConnectionClosed;

protected:

  typedef std::list<SocketConnection*> ConnectionList;
  ConnectionList connections_;
  SocketObserver* callback_;
};

//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_SOCKET_H_
