#ifndef HOTLINE_TUNNEL_SOCKET_H_
#define HOTLINE_TUNNEL_SOCKET_H_
#pragma once

#include <list>

#include "webrtc/base/stream.h"
#include "webrtc/base/socketaddress.h"
#include "webrtc/base/asyncsocket.h"
#include "webrtc/base/socketstream.h"
#include "webrtc/p2p/base/portinterface.h"
#include "webrtc/base/refcount.h"
#include "data_channel.h"


namespace hotline {

class SocketBase;
class HotlineDataChannel;


//////////////////////////////////////////////////////////////////////
  
class SocketObserver{
public:
  virtual void OnSocketOpen(SocketConnection* socket) = 0;
  virtual void OnSocketClosed(SocketConnection* socket) = 0;
};


//////////////////////////////////////////////////////////////////////


class SocketConnection : public sigslot::has_slots<> {
 public:
  enum { kBufferSize = 32 * 1024 };

  SocketConnection(SocketBase* server);
  virtual ~SocketConnection();

  bool AttachChannel(rtc::scoped_refptr<HotlineDataChannel> channel);
  rtc::scoped_refptr<HotlineDataChannel>  DetachChannel();
  void SetReady();
  void ReadEvent();

  void BeginProcess(rtc::StreamInterface* stream);
  rtc::StreamInterface* EndProcess();
  bool Send(const webrtc::DataBuffer& buffer);
  void Close();


 protected:

  class PacketQueue {
  public:
    PacketQueue();
    ~PacketQueue();

    size_t byte_count() const {
      return byte_count_;
    }
    bool Empty() const;
    webrtc::DataBuffer* Front();
    void Pop();
    void Push(webrtc::DataBuffer* packet);
    void Clear();
    void Swap(PacketQueue* other);
  private:
    std::deque<webrtc::DataBuffer*> packets_;
    size_t byte_count_;
  };

  void OnStreamEvent(rtc::StreamInterface* stream, int events, int error);
  void HandleStreamClose();

  void DoReceiveLoop();
  void flush_data();

  bool QueueSendDataMessage(const webrtc::DataBuffer& buffer);
  void SendQueuedDataMessages();

  SocketBase* server_;
  rtc::scoped_refptr<HotlineDataChannel> channel_;
  rtc::StreamInterface* stream_;
  bool closing_;
  bool is_ready_;

  PacketQueue queued_send_data_;
  char recv_buffer_[kBufferSize];
  size_t recv_len_;
};

//////////////////////////////////////////////////////////////////////

class SocketBase {
public:
  SocketBase();
  virtual ~SocketBase();

  void RegisterObserver(SocketObserver* callback);
  void UnregisterObserver();


  // Due to sigslot issues, we can't destroy some streams at an arbitrary time.
  sigslot::signal3<SocketBase*, SocketConnection*, rtc::StreamInterface*> SignalConnectionClosed;

protected:
  SocketConnection* HandleConnection(rtc::StreamInterface* stream);
  void Remove(SocketConnection* connection);

  typedef std::list<SocketConnection*> ConnectionList;
  ConnectionList connections_;
  SocketObserver* callback_;

  friend class SocketConnection;
};

//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_SOCKET_H_
