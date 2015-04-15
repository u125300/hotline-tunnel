#ifndef HOTLINE_TUNNEL_WEBSOCKET_H_
#define HOTLINE_TUNNEL_WEBSOCKET_H_
#pragma once

#include <string>
#include <vector>
#include "webrtc/base/sigslot.h"

struct libwebsocket;
struct libwebsocket_context;
struct libwebsocket_protocols;


namespace hotline {

class WsThreadHelper;
class WsMessage;

class WebSocket
{
public:
  WebSocket();
  virtual ~WebSocket();

  //
  // Data structure for message
  //
  struct Data
  {
    Data():bytes(nullptr), len(0), issued(0), isBinary(false){}
    char* bytes;
    size_t len, issued;
    bool isBinary;
  };

  //
  // Errors in websocket
  //
  enum class ErrorCode
  {
    TIME_OUT,
    CONNECTION_FAILURE,
    UNKNOWN,
  };

  //
  // Websocket state
  //
  enum class State
  {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED,
  };

  //
  // The signals to process websocket events.
  //

  sigslot::signal1<WebSocket*> SignalConnectEvent;
  sigslot::signal1<WebSocket*> SignalCloseEvent;
  sigslot::signal2<WebSocket*, const WebSocket::ErrorCode&> SignalErrorEvent;
  sigslot::signal2<WebSocket*, const Data&, sigslot::multi_threaded_local> SignalReadEvent;

  /**
   *  @brief  The initialized method for websocket.
   *          It needs to be invoked right after websocket instance is allocated.
   *  @param  delegate The delegate which want to receive event from websocket.
   *  @param  url      The URL of websocket server.
   *  @return true: Success, false: Failure
   */
  bool init(const std::string& url,
            const std::vector<std::string>* protocols = nullptr);

  /**
   *  @brief Sends string data to websocket server.
   */
  void send(const std::string& message);

  /**
   *  @brief Sends binary data to websocket server.
   */
  void send(const unsigned char* binaryMsg, unsigned int len);

  /**
   *  @brief Closes the connection to server.
   */
  void close();

  /**
   *  @brief Gets current state of connection.
   */
  State getReadyState();

private:
  virtual void onSubThreadStarted();
  virtual int onSubThreadLoop();
  virtual void onSubThreadEnded();

  friend class WebSocketCallbackWrapper;
  int onSocketCallback(struct libwebsocket_context *ctx,
                       struct libwebsocket *wsi,
                       int reason,
                       void *user, void *in, size_t len);

private:
  State        _readyState;
  std::string  _host;
  unsigned int _port;
  std::string  _path;

  size_t _pendingFrameDataLen;
  size_t _currentDataLen;
  char *_currentData;

  friend class WsThreadHelper;
  WsThreadHelper* _wsHelper;

  struct libwebsocket*         _wsInstance;
  struct libwebsocket_context* _wsContext;
  int _SSLConnection;
  struct libwebsocket_protocols* _wsProtocols;
};


//////////////////////////////////////////////////////////////////////

} // namespace hotline

#endif  // HOTLINE_TUNNEL_WEBSOCKET_H_
