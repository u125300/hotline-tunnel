#include "htn_config.h"

#include "webrtc/base/common.h"
#include "webrtc/base/thread.h"
#include "webrtc/base/asyncudpsocket.h"
#include "webrtc/system_wrappers/interface/sleep.h"

#ifdef WIN32
#include "webrtc/base/win32socketserver.h"
#endif

#include "socket.h"


namespace hotline {

static size_t kMaxQueuedSendDataBytes = 16 * 1024 * 1024;


///////////////////////////////////////////////////////////////////////////////
// SocketConnection::PacketQueue
///////////////////////////////////////////////////////////////////////////////

SocketConnection::PacketQueue::PacketQueue() : byte_count_(0) {}

SocketConnection::PacketQueue::~PacketQueue() {
  Clear();
}

bool SocketConnection::PacketQueue::Empty() const {
  return packets_.empty();
}

webrtc::DataBuffer* SocketConnection::PacketQueue::Front() {
  return packets_.front();
}

void SocketConnection::PacketQueue::Pop() {
  if (packets_.empty()) {
    return;
  }

  byte_count_ -= packets_.front()->size();
  packets_.pop_front();
}

void SocketConnection::PacketQueue::Push(webrtc::DataBuffer* packet) {
  byte_count_ += packet->size();
  packets_.push_back(packet);
}

void SocketConnection::PacketQueue::Clear() {
  while (!packets_.empty()) {
    delete packets_.front();
    packets_.pop_front();
  }
  byte_count_ = 0;
}

void SocketConnection::PacketQueue::Swap(PacketQueue* other) {
  size_t other_byte_count = other->byte_count_;
  other->byte_count_ = byte_count_;
  byte_count_ = other_byte_count;

  other->packets_.swap(packets_);
}


///////////////////////////////////////////////////////////////////////////////
// SocketConnection
///////////////////////////////////////////////////////////////////////////////

SocketConnection::SocketConnection(SocketBase* server)
  : server_(server), stream_(NULL), closing_(false), is_ready_(false), recv_len_(0) {
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

void SocketConnection::SetReady() {
  is_ready_ = true;
}

void SocketConnection::ReadEvent() {
  OnStreamEvent(stream_, rtc::SE_READ, 0);
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
  size_t pos = 0;
  int error;

  if (stream_->GetState() != rtc::SS_OPEN) {
    if (!QueueSendDataMessage(buffer)) {
      Close();
      return false;
    }
    return true;
  }

  if (buffer.size() == 0) {
    return true;
  }
  
  if (!queued_send_data_.Empty()) {
    if (!QueueSendDataMessage(buffer)) {
      Close();
      return false;
    }
    return true;
  }

  do{
    rtc::StreamResult write_result = stream_->Write( buffer.data.data()+pos,
                                                     buffer.size()-pos,
                                                     &written,
                                                     &error);
    ASSERT(write_result!=rtc::SR_ERROR);

    if (write_result == rtc::SR_SUCCESS) {
      if (buffer.size() < pos + written) {
        pos = pos + written;
        continue;
      }

      break;
    }
    else if (write_result == rtc::SR_BLOCK) {
      webrtc::SleepMs(1);
      continue;
    }
    else {
      // rtc::SR_EOS, rtc::SR_ERROR
      Close();
      return false;
    }
  }
  while (1);

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

  if (!channel_->IsOpen() ||  !is_ready_) {
    LOG(INFO) << "DoReceiveLoop() error";
    return;
  }

  LOG(INFO) << "DoReceiveLoop() passed";

  do{
    rtc::StreamResult read_result = stream_->Read(recv_buffer_,
                                                  sizeof(recv_buffer_),
                                                  &recv_len_,
                                                  &error);
    ASSERT(read_result!=rtc::SR_ERROR);

    if (read_result == rtc::SR_SUCCESS) {
      if (!channel_->Send(recv_buffer_, recv_len_)) {
        ASSERT(FALSE);
        Close();
        return;
      }
    }
    else if (read_result == rtc::SR_BLOCK) {
      break;
    }
    else {
      Close();
      return;
    }
  }
  while (true);

  return;
}

void SocketConnection::flush_data() {
  SendQueuedDataMessages();
}

bool SocketConnection::QueueSendDataMessage(const webrtc::DataBuffer& buffer) {
  if (queued_send_data_.byte_count() >= kMaxQueuedSendDataBytes) {
    LOG(LS_ERROR) << "Can't buffer any more data for the socket.";
    return false;
  }
  queued_send_data_.Push(new webrtc::DataBuffer(buffer));
  return true;
}

void SocketConnection::SendQueuedDataMessages() {
  
  size_t written;
  int error;

  while (!queued_send_data_.Empty()) {
    webrtc::DataBuffer* buffer = queued_send_data_.Front();

    do{
      rtc::StreamResult write_result = stream_->Write(buffer->data.data(),
                                                      buffer->size(),
                                                      &written,
                                                      &error);
      ASSERT(write_result!=rtc::SR_ERROR);

      if (write_result == rtc::SR_SUCCESS) {
        if (buffer->size() < written) {
          memmove(buffer->data.data(), buffer->data.data() + written, buffer->size()-written);
          buffer->data.SetLength(buffer->size()-written);
          continue;
        }
        break;
      }
      else if (write_result == rtc::SR_BLOCK) {
        webrtc::SleepMs(1);
        continue;
      }
      else {
        Close();
        return;
      }
    }
    while (1);

    queued_send_data_.Pop();
    delete buffer;
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

