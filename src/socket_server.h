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


namespace rtc {
  class ByteBuffer;
  class Thread;
}


namespace hotline {

class SocketServer;
class SocketServerConnectionInterface;
class SocketServerConnection;
class HotlineDataChannel;


//////////////////////////////////////////////////////////////////////
  
class SocketServerObserver{
public:
  virtual void OnSocketOpen(SocketServerConnection* socket) = 0;
  virtual void OnSocketClosed(SocketServerConnection* socket) = 0;

};


//////////////////////////////////////////////////////////////////////

class SocketServerConnection : public sigslot::has_slots<> {
 public:
  SocketServerConnection(SocketServer* server);
  virtual ~SocketServerConnection();

  bool AttachChannel(rtc::scoped_refptr<HotlineDataChannel> channel);
  rtc::scoped_refptr<HotlineDataChannel>  DetachChannel();

  void BeginProcess(rtc::StreamInterface* stream);
  rtc::StreamInterface* EndProcess();
  void Close();


 protected:
  void OnStreamEvent(rtc::StreamInterface* stream, int events, int error);
  void HandleStreamClose();

  SocketServer* server_;
  rtc::scoped_refptr<HotlineDataChannel> channel_;
  rtc::StreamInterface* stream_;
  bool closing_;
};

//////////////////////////////////////////////////////////////////////

class SocketServer {
public:
  SocketServer();
  virtual ~SocketServer();

  void RegisterObserver(SocketServerObserver* callback);
  void UnregisterObserver();

  bool HandleConnection(rtc::StreamInterface* stream);
  void Remove(SocketServerConnection* connection);

  // Due to sigslot issues, we can't destroy some streams at an arbitrary time.
  sigslot::signal3<SocketServer*, SocketServerConnection*, rtc::StreamInterface*> SignalConnectionClosed;

protected:

  typedef std::list<SocketServerConnection*> ConnectionList;
  ConnectionList connections_;
  SocketServerObserver* callback_;
};

//////////////////////////////////////////////////////////////////////

class SocketListenServer : public SocketServer, public sigslot::has_slots<> {
public:
  SocketListenServer();
  virtual ~SocketListenServer();

  bool Listen(const rtc::SocketAddress& address,
              const cricket::ProtocolType protocol);
  bool GetAddress(rtc::SocketAddress* address) const;
  void StopListening();

private:
  void OnReadEvent(rtc::AsyncSocket* socket);
  void OnConnectionClosed(SocketServer* server, SocketServerConnection* connection,
    rtc::StreamInterface* stream);

  rtc::scoped_ptr<rtc::AsyncSocket> listener_;
};

//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_SOCKET_SERVER_H_
