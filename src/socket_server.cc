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
// SocketServerConnection
///////////////////////////////////////////////////////////////////////////////

SocketServerConnection::SocketServerConnection(SocketServer* server)
  : server_(server), stream_(NULL), closing_(false) {
}


SocketServerConnection::~SocketServerConnection() {
  
}

bool SocketServerConnection::AttachChannel(rtc::scoped_refptr<HotlineDataChannel> channel) {
  if (channel_) return false;

  channel_ = channel;
  return true;
}

rtc::scoped_refptr<HotlineDataChannel> SocketServerConnection::DetachChannel() {
  rtc::scoped_refptr<HotlineDataChannel> channel = channel_;
  channel_ = NULL;
  return channel;
}

void SocketServerConnection::BeginProcess(rtc::StreamInterface* stream) {
  stream_ = stream;
  stream_->SignalEvent.connect(this, &SocketServerConnection::OnStreamEvent);
}


rtc::StreamInterface* SocketServerConnection::EndProcess(){
  rtc::StreamInterface* stream = stream_;
  stream_ = NULL;
  if (stream) stream->SignalEvent.disconnect(this);
  return stream;
}

void SocketServerConnection::Close() {
  HandleStreamClose();
}

void SocketServerConnection::HandleStreamClose(){

  if (closing_) return;
  closing_ = true;

  if (server_) {
    server_->Remove(this);
  }
}


void SocketServerConnection::OnStreamEvent(rtc::StreamInterface* stream,
                                           int events, int error) {
  if (events & rtc::SE_OPEN) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_OPEN.";
  }

  if (events & rtc::SE_READ) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_READ.";
  }

  if (events & rtc::SE_WRITE) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_WRITE.";
  }

  if (events & rtc::SE_CLOSE) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_CLOSE.";
    HandleStreamClose();
  }
}



///////////////////////////////////////////////////////////////////////////////
// SocketServer
///////////////////////////////////////////////////////////////////////////////

SocketServer::SocketServer() : callback_(NULL) {
}

SocketServer::~SocketServer() {
  for (ConnectionList::iterator it = connections_.begin();
       it != connections_.end();
       ++it) {
    rtc::StreamInterface* stream = (*it)->EndProcess();
    delete stream;
    delete *it;
  }
}

  
void SocketServer::RegisterObserver(SocketServerObserver* callback) {
  callback_ = callback;
}

void SocketServer::UnregisterObserver() {
  callback_ = NULL;
}

bool SocketServer::HandleConnection(rtc::StreamInterface* stream) {

  SocketServerConnection* connection = new SocketServerConnection(this);
  if (connection==NULL) return false;
  connections_.push_back(connection);

  // Notify to conductor
  callback_->OnSocketOpen(connection);
  
  // Begin process
  connection->BeginProcess(stream);
  return true;
}


void SocketServer::Remove(SocketServerConnection* connection) {
  // Notify to conductor
  callback_->OnSocketClosed(connection);

  connections_.remove(connection);
  SignalConnectionClosed(this, connection, connection->EndProcess());
  delete connection;
}

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
#ifdef WIN32
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    if (!sock->CreateT(address.family(), SOCK_DGRAM)){
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      delete sock;
      return false;
    }
    listener_.reset(sock);
#elif defined(POSIX)
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
#ifdef WIN32
    rtc::Win32Socket* sock = new rtc::Win32Socket();
    if (!sock->CreateT(address.family(), SOCK_STREAM)) {
      LOG(LS_ERROR) << "Local port already in use or no privilege to bind port.";
      delete sock;
      return false;
    }

    listener_.reset(sock);
#elif defined(POSIX)
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

void SocketListenServer::OnConnectionClosed(SocketServer* server,
            SocketServerConnection* connection,
            rtc::StreamInterface* stream) {
  rtc::Thread::Current()->Dispose(stream);
}

///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

