#ifndef HOTLINE_TUNNEL_SOCKET_LISTENING_H_
#define HOTLINE_TUNNEL_SOCKET_LISTENING_H_
#pragma once

#include <list>

#include "webrtc/base/stream.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/socketstream.h"
#include "webrtc/p2p/base/portinterface.h"



class SocketListening : public sigslot::has_slots<>,
                               public rtc::MessageHandler {
public:

  typedef std::vector<rtc::scoped_ptr<rtc::SocketStream>> SocketStreams;

  explicit SocketListening();
  ~SocketListening();

  bool Listen(cricket::ProtocolType proto, rtc::SocketAddress &address);

private:
 
  enum { kBufferSize = 64 * 1024 };

  // tcp only
  void OnNewConnection(rtc::AsyncSocket* socket);
  void AcceptConnection(rtc::AsyncSocket* server_socket);

  // tcp and udp
  void OnReadEvent(rtc::AsyncSocket* socket);
  void OnStreamEvent(rtc::StreamInterface* stream, int events, int error);

  bool DoReceiveLoop(rtc::StreamInterface* stream);
  void HandleStreamClose(rtc::StreamInterface* stream);
  void flush_data();

  // MessageHandler
  void OnMessage(rtc::Message* msg) {}


  // buffers, will be changed to heap.
  char buffer_[kBufferSize];
  size_t len_;

  rtc::SocketAddress listening_address_;
  cricket::ProtocolType proto_;

  rtc::scoped_ptr<rtc::AsyncSocket> listener_;
  SocketStreams socket_streams_;
};



class HTunnelSocketConnection {

public:

protected:
};



#endif  // HOTLINE_TUNNEL_SOCKET_LISTENING_H_
