#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"

#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif

#include "socket.h"


namespace hotline {


///////////////////////////////////////////////////////////////////////////////
// SocketConnection
///////////////////////////////////////////////////////////////////////////////

SocketConnection::SocketConnection(SocketBase* server)
  : server_(server), stream_(NULL), closing_(false), recv_len_(0), send_len_(0) {
}


SocketConnection::~SocketConnection() {
  
}

bool SocketConnection::AttachChannel(rtc::scoped_refptr<HotlineDataChannel> channel) {
  if (channel_) return false;

  channel_ = channel;
  return true;
}

rtc::scoped_refptr<HotlineDataChannel> SocketConnection::DetachChannel() {
  rtc::scoped_refptr<HotlineDataChannel> channel = channel_;
  channel_ = NULL;
  return channel;
}

void SocketConnection::BeginProcess(rtc::StreamInterface* stream) {
  stream_ = stream;
  stream_->SignalEvent.connect(this, &SocketConnection::OnStreamEvent);
}


rtc::StreamInterface* SocketConnection::EndProcess(){
  rtc::StreamInterface* stream = stream_;
  stream_ = NULL;
  if (stream) stream->SignalEvent.disconnect(this);
  return stream;
}

bool SocketConnection::Send(const webrtc::DataBuffer& buffer) {

  size_t written;
  int error;

  if (send_len_>0) return false;

  rtc::StreamResult write_result = stream_->Write(buffer.data.data(), buffer.size(), &written, &error);
  
  switch (write_result) {
  case rtc::SR_SUCCESS:
    send_len_ = buffer.size()-written;
    if (send_len_>0)
      memcpy(send_buffer_, buffer.data.data() + written , send_len_);
    break;

  case rtc::SR_BLOCK:
    ASSERT(FALSE);
    send_len_ = buffer.size();
    memcpy(send_buffer_, buffer.data.data() , send_len_);
    break;

  case rtc::SR_EOS:
  case rtc::SR_ERROR:
    Close();
    return false;

  }

  return true;
}

void SocketConnection::Close() {
  HandleStreamClose();
}

void SocketConnection::HandleStreamClose(){
  if (closing_) return;
  closing_ = true;

  if (server_) {
    server_->Remove(this);
  }
}


void SocketConnection::OnStreamEvent(rtc::StreamInterface* stream,
                                           int events, int error) {
  if (events & rtc::SE_OPEN) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_OPEN.";
  }

  if (events & rtc::SE_READ) {
    DoReceiveLoop();
  }

  if (events & rtc::SE_WRITE) {
    flush_data();
  }

  if (events & rtc::SE_CLOSE) {
    LOG(INFO) << __FUNCTION__ << " " << " rtc::SE_CLOSE.";
    HandleStreamClose();
  }
}

void SocketConnection::DoReceiveLoop() {

  int error;

  if (recv_len_==0) {
    rtc::StreamResult read_result = stream_->Read(recv_buffer_, sizeof(recv_buffer_), &recv_len_, &error);
  
    switch (read_result) {
    case rtc::SR_SUCCESS:
      ASSERT(recv_len_ <= sizeof(recv_buffer_));
      break;

    case rtc::SR_BLOCK:
      break;

    case rtc::SR_EOS:
    case rtc::SR_ERROR:
      Close();
      return;
    }
  }

  if (recv_len_>0) {
    if (!channel_->Send(recv_buffer_, recv_len_)) {
      ASSERT(FALSE);
      return;
    }

    recv_len_ = 0;
  }

  return;
}


void SocketConnection::flush_data() {
  
  if (send_len_==0) return;

  size_t written;
  int error;
  rtc::StreamResult write_result = stream_->Write(send_buffer_, send_len_, &written, &error);
  if (write_result == rtc::SR_SUCCESS) {
    send_len_ -= written;
    if (send_len_ > 0) {
      ASSERT(FALSE);
      memmove(send_buffer_, send_buffer_+written, send_len_);
    }
  }
  else {
    ASSERT(FALSE);
  }
}



///////////////////////////////////////////////////////////////////////////////
// SocketBase
///////////////////////////////////////////////////////////////////////////////

SocketBase::SocketBase() : callback_(NULL) {
}

SocketBase::~SocketBase() {
  for (ConnectionList::iterator it = connections_.begin();
       it != connections_.end();
       ++it) {
    rtc::StreamInterface* stream = (*it)->EndProcess();
    delete stream;
    delete *it;
  }
}

  
void SocketBase::RegisterObserver(SocketObserver* callback) {
  callback_ = callback;
}

void SocketBase::UnregisterObserver() {
  callback_ = NULL;
}

SocketConnection* SocketBase::HandleConnection(rtc::StreamInterface* stream) {

  SocketConnection* connection = new SocketConnection(this);
  if (connection==NULL) return NULL;
  connections_.push_back(connection);

  // Notify to conductor
  callback_->OnSocketOpen(connection);
  
  // Begin process
  connection->BeginProcess(stream);
  return connection;
}


void SocketBase::Remove(SocketConnection* connection) {

  // Notify to conductor
  callback_->OnSocketClosed(connection);

  connections_.remove(connection);
  SignalConnectionClosed(this, connection, connection->EndProcess());
  delete connection;
}


///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

