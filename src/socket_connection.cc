#include "htn_config.h"

#include "webrtc/base//common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"
#include "socket_connection.h"

#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif



SocketConnection::SocketConnection( bool server_mode) :
      callback_(NULL),
      server_mode_(server_mode),
      state_(SC_NOT_STARTED) {
}

SocketConnection::~SocketConnection() {

}


void SocketConnection::InitSocketSignals() {

  // TODO: Not implemented yet

}


void SocketConnection::RegisterObserver(SocketConnectionObserver* callback) {
  ASSERT(!callback_);
  callback_ = callback;
}


bool SocketConnection::SetServerMode() {
  if (server_mode_) return true;

  LOG(LS_ERROR) << "Fail to initialize server mode.";
  return false;
}


bool SocketConnection::SetClientMode(rtc::SocketAddress local_address,
                                     rtc::SocketAddress remote_address,
                                     std::string tunnel_key,
                                     cricket::ProtocolType protocol) {
  if (server_mode_) {
    LOG(LS_ERROR) << "Fail to initialize client mode.";
    return false;
  }

  local_address_ = local_address;
  remote_address_ = remote_address;
  tunnel_key_ = tunnel_key;
  protocol_ = protocol;

  return true;
}

bool SocketConnection::StartServerMode() {
  // Do nothing yet
  return true;
}


bool SocketConnection::StartClientMode() {

  // This function should not be blocked and run on signal thread.

  //
  // Listen to local port
  //

  if (!StartListening()) {
    return false;
  }

  return false;
}



  
bool SocketConnection::StartListening() {

  if (state_ == SC_STARTED) return true;

  //
  // UDP socket
  //
  if (protocol_ == cricket::PROTO_UDP) {
#ifdef WIN32
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    if (!sock->CreateT(local_address_.family(), SOCK_DGRAM)){
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      delete sock;
      return false;
    }
    listening_.reset(sock);
#elif defined(POSIX)
    rtc::Thread* thread = rtc::Thread::Current();
    ASSERT(thread != NULL);
    listening_.reset(thread->socketserver()->CreateAsyncSocket(address.family(), SOCK_DGRAM));
#else
#error Platform not supported.
#endif

    if ((listening_->Bind(local_address_) == SOCKET_ERROR)) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      return false;
    }

    rtc::SocketStream *stream = new rtc::SocketStream(listening_.get());
    if (stream == NULL) return false;
    stream->SignalEvent.connect(this, &SocketConnection::OnStreamEvent);
    streams_.push_back(rtc::scoped_ptr<rtc::SocketStream>(stream));
  }

  //
  // TCP socket
  //
  else if (protocol_ == cricket::PROTO_TCP) {
#ifdef WIN32
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    if (!sock->CreateT(local_address_.family(), SOCK_STREAM)) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      delete sock;
      return false;
    }

    listening_.reset(sock);
#elif defined(POSIX)
    rtc::Thread* thread = rtc::Thread::Current();
    ASSERT(thread != NULL);
    listener_.reset(thread->socketserver()->CreateAsyncSocket(address.family(), SOCK_STREAM));
#else
#error Platform not supported.
#endif

    listening_->SignalReadEvent.connect(this, &SocketConnection::OnNewConnection);

    if ((listening_->Bind(local_address_) == SOCKET_ERROR) ||
      (listening_->Listen(5) == SOCKET_ERROR)) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      return false;
    }

    if (listening_->GetError() != 0) return false;
  }

  state_ = SC_STARTED;
  return true;
}


void SocketConnection::OnNewConnection(rtc::AsyncSocket* socket) {
  AcceptConnection(socket);
}


void SocketConnection::AcceptConnection(rtc::AsyncSocket* server_socket) {

  ASSERT(server_socket == listening_.get());
  ASSERT(listening_);

  rtc::AsyncSocket* accepted_socket = server_socket->Accept(NULL);

  if (accepted_socket) {
    rtc::SocketStream* stream = new rtc::SocketStream(accepted_socket);
    stream->SignalEvent.connect(this, &SocketConnection::OnStreamEvent);
    streams_.push_back(rtc::scoped_ptr<rtc::SocketStream>(stream));
    callback_->OnSocketConnected();
  }
}


void SocketConnection::OnStreamEvent(rtc::StreamInterface* stream,
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


void SocketConnection::HandleStreamClose(rtc::StreamInterface* stream) {
  if (stream != NULL) {
    stream->Close();
  }
}


bool
SocketConnection::DoReceiveLoop(rtc::StreamInterface* stream) {

  // TODO: Not implemented

  return false;
}



void
SocketConnection::flush_data() {

  // TODO: Not implemented

}
