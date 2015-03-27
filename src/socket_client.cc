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

bool SocketClient::Connect(const rtc::SocketAddress& address,
  const cricket::ProtocolType protocol) {

#ifdef WIN32
  rtc::Win32Socket* sock = new rtc::Win32Socket();

  if (!sock->CreateT(address.family(),
                    protocol==cricket::PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM)){
    LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
    delete sock;
    return false;
  }
  connector_.reset(sock);
#elif defined(POSIX)
  rtc::Thread* thread = rtc::Thread::Current();
  ASSERT(thread != NULL);
  rtc::AsyncSocket* sock = thread->socketserver()->CreateAsyncSocket(address.family(), 
                                protocol==cricket::PROTO_UDP ? SOCK_DGRAM : SOCK_STREAM);
  if (!sock) {
    LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
    return false;
  }
  connector_.reset(sock);
#else
#error Platform not supported.
#endif
  
  ASSERT(connector_->GetState() == rtc::Socket::CS_CLOSED);
  int err = connector_->Connect(address);
  if (err == SOCKET_ERROR) {
    return false;
  }

  rtc::StreamInterface *stream = new rtc::SocketStream(connector_.get());
  if (stream == NULL) return false;
  HandleConnection(stream);

  return true;
}


void SocketClient::Disconnect() {
  
}


SocketConnection* SocketClient::GetConnection() {
  ASSERT(connections_.size()<=1);

  if (connections_.size()==0) return NULL;
  return connections_.front();
}


void SocketClient::OnConnectionClosed(SocketBase* server,
            SocketConnection* connection,
            rtc::StreamInterface* stream) {
  rtc::Thread::Current()->Dispose(stream);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

