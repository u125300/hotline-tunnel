#ifndef HOTLINE_TUNNEL_SOCKET_CONNECTION_H_
#define HOTLINE_TUNNEL_SOCKET_CONNECTION_H_
#pragma once

#include <list>

#include "webrtc/base/stream.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/socketstream.h"
#include "webrtc/p2p/base/portinterface.h"




struct SocketConnectionObserver{
  virtual void OnSocketConnected() = 0;
  virtual void OnSocketDisconnected()= 0;
  virtual void OnMessageFromSocket() = 0;
  virtual void OnMessageToSocket() = 0;

protected:
  virtual ~SocketConnectionObserver() {}
};


class SocketConnection : public sigslot::has_slots<>,
                         public rtc::MessageHandler {
public:
  typedef std::vector<rtc::scoped_ptr<rtc::SocketStream>> SocketStreams;

  SocketConnection();
  ~SocketConnection();

  void RegisterObserver(SocketConnectionObserver* callback);

  bool Listen(cricket::ProtocolType proto,
              rtc::SocketAddress &address);

  // implements the MessageHandler interface
  void OnMessage(rtc::Message* msg) {}

protected:
 
  enum { kBufferSize = 64 * 1024 };

  void InitSocketSignals();

  //
  // Listening socket
  //

  // tcp only
  void OnNewConnection(rtc::AsyncSocket* socket);
  void AcceptConnection(rtc::AsyncSocket* server_socket);

  // tcp and udp
  void OnReadEvent(rtc::AsyncSocket* socket);
  void OnStreamEvent(rtc::StreamInterface* stream, int events, int error);

  bool DoReceiveLoop(rtc::StreamInterface* stream);
  void HandleStreamClose(rtc::StreamInterface* stream);
  void flush_data();

  //
  // Connection socket
  //

  // TODO: not implemented


  //
  // Member variables
  //

  SocketConnectionObserver* callback_;
  rtc::scoped_ptr<rtc::AsyncSocket> listening_;
  rtc::scoped_ptr<rtc::AsyncSocket> connection_;
  SocketStreams streams_;
  rtc::SocketAddress address_;
  cricket::ProtocolType proto_;
  char buffer_[kBufferSize];
  size_t len_;
};


#endif  // HOTLINE_TUNNEL_SOCKET_CONNECTION_H_