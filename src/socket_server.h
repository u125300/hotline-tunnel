#ifndef HOTLINE_TUNNEL_SOCKET_SERVER_H_
#define HOTLINE_TUNNEL_SOCKET_SERVER_H_
#pragma once

#include <list>

#include "webrtc/base/stream.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/socketstream.h"
#include "webrtc/p2p/base/portinterface.h"
#include "webrtc/base/refcount.h"
#include "data_channel.h"
#include "socket.h"


namespace rtc {
  class ByteBuffer;
  class Thread;
}


namespace hotline {

class HotlineDataChannel;


//////////////////////////////////////////////////////////////////////

class SocketListenServer : public SocketBase, public sigslot::has_slots<> {
public:
  SocketListenServer();
  virtual ~SocketListenServer();

  bool Listen(const rtc::SocketAddress& address,
              const cricket::ProtocolType protocol);
  bool GetAddress(rtc::SocketAddress* address) const;
  void StopListening();

private:
  void OnReadEvent(rtc::AsyncSocket* socket);
  void OnConnectionClosed(SocketBase* server, SocketConnection* connection,
    rtc::StreamInterface* stream);

  rtc::scoped_ptr<rtc::AsyncSocket> listener_;
};

//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_SOCKET_SERVER_H_
