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


namespace rtc {
  class ByteBuffer;
  class Thread;
}


namespace hotline {

class SocketServer;
class SocketServerConnectionInterface;
class SocketServerConnection;

//////////////////////////////////////////////////////////////////////

class SocketServerConnectionObserver
  : public rtc::RefCountInterface {
public:

  SocketServerConnectionObserver(SocketServerConnectionInterface* connection);
  virtual ~SocketServerConnectionObserver();

protected:
  SocketServerConnectionInterface* connection_;
};

//////////////////////////////////////////////////////////////////////

class SocketServerConnectionInterface {

 public:
  virtual void RegisterObserver(SocketServerConnectionObserver* observer) = 0;
  virtual void UnregisterObserver() = 0;

};

//////////////////////////////////////////////////////////////////////

class SocketServerConnection : public SocketServerConnectionInterface,
                               public sigslot::has_slots<> {
 public:
  SocketServerConnection(SocketServer* server);
  virtual ~SocketServerConnection();

  void RegisterObserver(SocketServerConnectionObserver* observer);
  void UnregisterObserver();

  void BeginProcess(rtc::StreamInterface* stream);
  rtc::StreamInterface* EndProcess();


 protected:
  void OnStreamEvent(rtc::StreamInterface* stream, int events, int error);

  SocketServer* server_;
  SocketServerConnectionObserver* observer_;
  rtc::StreamInterface* stream_;

};

//////////////////////////////////////////////////////////////////////

class SocketServer {
public:
  SocketServer();
  virtual ~SocketServer();

  bool HandleConnection(rtc::StreamInterface* stream);

  // Due to sigslot issues, we can't destroy some streams at an arbitrary time.
  sigslot::signal3<SocketServer*, SocketServerConnection*, rtc::StreamInterface*> SignalConnectionClosed;

private:
  void Remove(SocketServerConnection* connection);

  typedef std::list<SocketServerConnection*> ConnectionList;
  ConnectionList connections_;
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
