#include "htn_config.h"

#include "webrtc/base//common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"
#include "socket_listening.h"

#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif

SocketListening::SocketListening() {
}

SocketListening::~SocketListening() {

}

bool SocketListening::Listen(cricket::ProtocolType proto,
  rtc::SocketAddress &address) {

  proto_ = proto;

  //
  // UDP socket
  //
  if (proto == cricket::PROTO_UDP) {
#ifdef WIN32
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    sock->CreateT(address.family(), SOCK_DGRAM);
    listener_.reset(sock);
#elif defined(POSIX)
    rtc::Thread* thread = rtc::Thread::Current();
    ASSERT(thread != NULL);
    listener_.reset(thread->socketserver()->CreateAsyncSocket(address.family(), SOCK_DGRAM));
#else
#error Platform not supported.
#endif
    rtc::SocketStream *stream = new rtc::SocketStream(listener_.get());
    if (stream == NULL) return false;
    stream->SignalEvent.connect(this, &SocketListening::OnStreamEvent);
    socket_streams_.push_back(rtc::scoped_ptr<rtc::SocketStream>(stream));
  }

  //
  // TCP socket
  //
  else if (proto == cricket::PROTO_TCP) {
#ifdef WIN32
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    sock->CreateT(address.family(), SOCK_STREAM);
    listener_.reset(sock);
#elif defined(POSIX)
    rtc::Thread* thread = rtc::Thread::Current();
    ASSERT(thread != NULL);
    listener_.reset(thread->socketserver()->CreateAsyncSocket(address.family(), SOCK_STREAM));
#else
#error Platform not supported.
#endif

    listener_->SignalReadEvent.connect(this, &SocketListening::OnNewConnection);

    if ((listener_->Bind(address) == SOCKET_ERROR) ||
       (listener_->Listen(5) == SOCKET_ERROR)) {
      return false;
    }

    if (listener_->GetError() != 0) return false;
  }

  return true;
}


void SocketListening::OnNewConnection(rtc::AsyncSocket* socket) {
  AcceptConnection(socket);
}


void SocketListening::AcceptConnection(rtc::AsyncSocket* server_socket) {

  ASSERT(server_socket == listener_.get());
  ASSERT(listener_);

  rtc::AsyncSocket* accepted_socket = server_socket->Accept(NULL);

  if (accepted_socket) {
    rtc::SocketStream* stream = new rtc::SocketStream(accepted_socket);
    stream->SignalEvent.connect(this, &SocketListening::OnStreamEvent);
    socket_streams_.push_back(rtc::scoped_ptr<rtc::SocketStream>(stream));

  }
}



void SocketListening::OnStreamEvent(rtc::StreamInterface* stream,
  int events, int error) {

  if (events & rtc::SE_OPEN) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_OPEN.";
  }

  if (events & rtc::SE_READ) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_READ.";
    len_ = 0;
    DoReceiveLoop(stream);
  }

  if (events & rtc::SE_WRITE) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_WRITE.";
  }

  if (events & rtc::SE_CLOSE) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_CLOSE.";
    HandleStreamClose(stream);
  }
}


void SocketListening::HandleStreamClose(rtc::StreamInterface* stream) {
  if (stream != NULL) {
    stream->Close();
  }
}


bool
SocketListening::DoReceiveLoop(rtc::StreamInterface* stream) {

  // Not implemented

  return false;
}



void
SocketListening::flush_data() {

  // Not implemented

}
