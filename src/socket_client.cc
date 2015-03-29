#include "htn_config.h"

#include "webrtc/base//common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"
#include "socket_client.h"

#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif


namespace hotline {

///////////////////////////////////////////////////////////////////////////////
// SocketListenServer
///////////////////////////////////////////////////////////////////////////////

SocketClient::SocketClient() {
  SignalConnectionClosed.connect(this, &SocketClient::OnConnectionClosed);
}

SocketClient::~SocketClient() {
}

SocketConnection* SocketClient::Connect(const rtc::SocketAddress& address,
                           const cricket::ProtocolType protocol) {

#ifdef WIN32
  rtc::Win32Socket* sock = new rtc::Win32Socket();

  if (!sock->CreateT(address.family(),
                    protocol==cricket::PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM)){
    LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
    delete sock;
    return NULL;
  }
#elif defined(POSIX)
  rtc::Thread* thread = rtc::Thread::Current();
  ASSERT(thread != NULL);
  rtc::AsyncSocket* sock = thread->socketserver()->CreateAsyncSocket(address.family(), 
                                protocol==cricket::PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM);
  if (!sock) {
    LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
    return false;
  }
#else
#error Platform not supported.
#endif
  
  ASSERT(sock->GetState() == rtc::Socket::CS_CLOSED);
  int err = sock->Connect(address);
  if (err == SOCKET_ERROR) {
    return NULL;
  }

  rtc::StreamInterface *stream = new rtc::SocketStream(sock);
  if (stream == NULL) return NULL;

  SocketConnection* connection = HandleConnection(stream);
  return connection;
}


void SocketClient::Disconnect() {
  
}


void SocketClient::OnConnectionClosed(SocketBase* server,
            SocketConnection* connection,
            rtc::StreamInterface* stream) {
  rtc::Thread::Current()->Dispose(stream);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

