#include "htn_config.h"

#include "webrtc/base//common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"
#include "socket_server.h"

#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif


namespace hotline {


///////////////////////////////////////////////////////////////////////////////
// SocketListenServer
///////////////////////////////////////////////////////////////////////////////

SocketListenServer::SocketListenServer() {
  SignalConnectionClosed.connect(this, &SocketListenServer::OnConnectionClosed);
}

SocketListenServer::~SocketListenServer() {
}

bool SocketListenServer::Listen(const rtc::SocketAddress& address,
                               const cricket::ProtocolType protocol) {
  //
  // UDP socket
  //
  if (protocol == cricket::PROTO_UDP) {
#if defined(WEBRTC_WIN)
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    if (!sock->CreateT(address.family(), SOCK_DGRAM)){
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      delete sock;
      return false;
    }
    listener_.reset(sock);
#elif defined(WEBRTC_POSIX)
    rtc::Thread* thread = rtc::Thread::Current();
    ASSERT(thread != NULL);
    rtc::AsyncSocket* sock = thread->socketserver()->CreateAsyncSocket(address.family(), SOCK_DGRAM);
    if (!sock) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      return false;
    }
    listener_.reset(sock);
#else
#error Platform not supported.
#endif

    if ((listener_->Bind(address) == SOCKET_ERROR)) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      return false;
    }

    rtc::StreamInterface *stream = new rtc::SocketStream(listener_.get());
    if (stream == NULL) return false;
    HandleConnection(stream);
  }

  //
  // TCP socket
  //
  else if (protocol == cricket::PROTO_TCP) {
#if defined(WEBRTC_WIN)
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    if (!sock->CreateT(address.family(), SOCK_STREAM)) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      delete sock;
      return false;
    }

    listener_.reset(sock);
#elif defined(WEBRTC_POSIX)
    rtc::Thread* thread = rtc::Thread::Current();
    ASSERT(thread != NULL);
    rtc::AsyncSocket* sock = thread->socketserver()->CreateAsyncSocket(address.family(),
                                                                       SOCK_STREAM);
    if (!sock) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      return false;
    }

    listener_.reset(sock);
#else
#error Platform not supported.
#endif

    listener_->SignalReadEvent.connect(this, &SocketListenServer::OnReadEvent);

    if ((listener_->Bind(address) == SOCKET_ERROR) ||
      (listener_->Listen(5) == SOCKET_ERROR)) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      return false;
    }

    if (listener_->GetError() != 0) return false;
  }

  return true;

}

bool SocketListenServer::GetAddress(rtc::SocketAddress* address) const {
  if (!listener_) {
    return false;
  }
  *address = listener_->GetLocalAddress();
  return !address->IsNil();
}

void SocketListenServer::StopListening() {
  if (listener_) {
    listener_->Close();
  }
}

void SocketListenServer::OnReadEvent(rtc::AsyncSocket* socket) {
  ASSERT(socket == listener_.get());
  ASSERT(listener_);
  rtc::AsyncSocket* incoming = listener_->Accept(NULL);
  if (incoming) {
    rtc::StreamInterface* stream = new rtc::SocketStream(incoming);
    //stream = new LoggingAdapter(stream, LS_VERBOSE, "SocketServer", false);
    HandleConnection(stream);
  }
}

void SocketListenServer::OnConnectionClosed(SocketBase* server,
            SocketConnection* connection,
            rtc::StreamInterface* stream) {
  rtc::Thread::Current()->Dispose(stream);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

