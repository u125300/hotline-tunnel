#include "htn_config.h"

#include <list>
#include <mutex>
#include <thread>
#include <algorithm>


#include "webrtc/base/refcount.h"
#include "webrtc/base/common.h"
#include "webrtc/system_wrappers/interface/trace.h"
#ifdef ARRAY_SIZE
#undef ARRAY_SIZE
#endif
#include "libwebsockets.h"
#include "websocket.h"


namespace hotline {

#define WS_WRITE_BUFFER_SIZE 2048

#define CC_SAFE_DELETE(p)           do { if(p) { delete (p); (p) = nullptr; } } while(0)
#define CC_SAFE_DELETE_ARRAY(p)     do { if(p) { delete[] (p); (p) = nullptr; } } while(0)

  
class WsMessage
{
public:
    WsMessage() : what(0), obj(nullptr){}
    unsigned int what; // message type
    void* obj;
};

/**
 *  @brief Websocket thread helper, it's used for sending message between caller and websocket thread.
 */
class WsThreadHelper
{
public:
  WsThreadHelper();
  ~WsThreadHelper();
        
  // Creates a new thread
  bool createThread(const WebSocket& ws);
  // Quits sub-thread (websocket thread).
  void quitSubThread();
    
  // Sends message to sub-thread(websocket thread). It's needs to be invoked in UI thread.
  void sendMessageToSubThread(WsMessage *msg);
    
  // Waits the sub-thread (websocket thread) to exit,
  void joinSubThread();
    
protected:
  void wsThreadEntryFunc();
    
private:
  std::list<WsMessage*>* _subThreadWsMessageQueue;
  std::mutex   _subThreadWsMessageQueueMutex;
  std::thread* _subThreadInstance;
  WebSocket* _ws;
  bool _needQuit;
  friend class WebSocket;
};


// Wrapper for converting websocket callback from static function to member function of WebSocket class.
class WebSocketCallbackWrapper {
public:
    
  static int onSocketCallback(struct libwebsocket_context *ctx,
                              struct libwebsocket *wsi,
                              enum libwebsocket_callback_reasons reason,
                              void *user, void *in, size_t len)
  {
    // Gets the user data from context. We know that it's a 'WebSocket' instance.
    WebSocket* wsInstance = (WebSocket*)libwebsocket_context_user(ctx);
    if (wsInstance) {
      return wsInstance->onSocketCallback(ctx, wsi, reason, user, in, len);
    }
    return 0;
  }
};

// Implementation of WsThreadHelper
WsThreadHelper::WsThreadHelper()
: _subThreadInstance(nullptr)
, _ws(nullptr)
, _needQuit(false)
{
    _subThreadWsMessageQueue = new std::list<WsMessage*>();
}

WsThreadHelper::~WsThreadHelper()
{
  joinSubThread();
  CC_SAFE_DELETE(_subThreadInstance);
  delete _subThreadWsMessageQueue;
}

bool WsThreadHelper::createThread(const WebSocket& ws)
{
    _ws = const_cast<WebSocket*>(&ws);
    
    // Creates websocket thread
    _subThreadInstance = new std::thread(&WsThreadHelper::wsThreadEntryFunc, this);
    return true;
}

void WsThreadHelper::quitSubThread()
{
    _needQuit = true;
}

void WsThreadHelper::wsThreadEntryFunc()
{
  _ws->onSubThreadStarted();

  while (!_needQuit) {
    if (_ws->onSubThreadLoop()) {
      break;
    }
  }
}

void WsThreadHelper::sendMessageToSubThread(WsMessage *msg)
{
  std::lock_guard<std::mutex> lk(_subThreadWsMessageQueueMutex);
  _subThreadWsMessageQueue->push_back(msg);
}

void WsThreadHelper::joinSubThread()
{
  if (_subThreadInstance->joinable()) {
    _subThreadInstance->join();
  }
}

enum WS_MSG {
    WS_MSG_TO_SUBTRHEAD_SENDING_STRING = 0,
    WS_MSG_TO_SUBTRHEAD_SENDING_BINARY
};


WebSocket::WebSocket()
    : _readyState(State::CLOSED)
    , _port(80)
    , _pendingFrameDataLen(0)
    , _currentDataLen(0)
    , _currentData(nullptr)
    , _wsHelper(nullptr)
    , _wsInstance(nullptr)
    , _wsContext(nullptr)
    , _SSLConnection(0)
    , _wsProtocols(nullptr)
{
}

WebSocket::~WebSocket()
{
  close();
  CC_SAFE_DELETE(_wsHelper);
    
  if (_wsProtocols) {
    for (int i = 0; _wsProtocols[i].callback != nullptr; ++i) {
      CC_SAFE_DELETE_ARRAY(_wsProtocols[i].name);
    }

    CC_SAFE_DELETE_ARRAY(_wsProtocols);
  }
}

bool WebSocket::init(const std::string& url,
                     const std::vector<std::string>* protocols/* = nullptr*/)
{
  bool ret = false;
  bool useSSL = false;
  std::string host = url;
  size_t pos = 0;
  int port = 80;
    
  //ws://
  pos = host.find("ws://");
  if (pos == 0) host.erase(0,5);
    
  pos = host.find("wss://");
  if (pos == 0)
  {
    host.erase(0,6);
    useSSL = true;
  }
    
  pos = host.find(":");
  if (pos != std::string::npos) port = atoi(host.substr(pos+1, host.size()).c_str());
    
  pos = host.find("/", 0);
  std::string path = "/";
  if (pos != std::string::npos) path += host.substr(pos + 1, host.size());
    
  pos = host.find(":");
  if(pos != std::string::npos){
      host.erase(pos, host.size());
  }else if((pos = host.find("/")) != std::string::npos) {
  	host.erase(pos, host.size());
  }
    
  _host = host;
  _port = port;
  _path = path;
  //: _SSLConnection = useSSL ? 1 : 0;
  _SSLConnection = useSSL ? 2 : 0;
  
  webrtc::WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceUndefined, -1,
                       "[WebSocket::init] _host: %s, _port: %d, _path: %s", _host.c_str(), _port, _path.c_str());

  size_t protocolCount = 0;
  if (protocols && protocols->size() > 0)
  {
    protocolCount = protocols->size();
  }
  else
  {
    protocolCount = 1;
  }
    
	_wsProtocols = new libwebsocket_protocols[protocolCount+1];
	memset(_wsProtocols, 0, sizeof(libwebsocket_protocols)*(protocolCount+1));

  if (protocols && protocols->size() > 0) {
    int i = 0;
    for (std::vector<std::string>::const_iterator iter = protocols->begin(); iter != protocols->end(); ++iter, ++i)
    {
      char* name = new char[(*iter).length()+1];
      strcpy(name, (*iter).c_str());
      _wsProtocols[i].name = name;
      _wsProtocols[i].callback = WebSocketCallbackWrapper::onSocketCallback;
    }
  }
  else {
    char* name = new char[20];
    strcpy(name, "default-protocol");
    _wsProtocols[0].name = name;
    _wsProtocols[0].callback = WebSocketCallbackWrapper::onSocketCallback;
  }
    
  // WebSocket thread needs to be invoked at the end of this method.
  _wsHelper = new (std::nothrow) WsThreadHelper();
  ret = _wsHelper->createThread(*this);
    
  return ret;
}

void WebSocket::send(const std::string& message)
{
  if (_readyState == State::OPEN) {
    // In main thread
    WsMessage* msg = new (std::nothrow) WsMessage();
    msg->what = WS_MSG_TO_SUBTRHEAD_SENDING_STRING;
    Data* data = new (std::nothrow) Data();
    data->bytes = new char[message.length()+1];
    strcpy(data->bytes, message.c_str());
    data->len = static_cast<size_t>(message.length());
    msg->obj = data;
    _wsHelper->sendMessageToSubThread(msg);
  }
}

void WebSocket::send(const unsigned char* binaryMsg, unsigned int len)
{
  ASSERT(binaryMsg != nullptr && len > 0);  // parameter invalid.

  if (_readyState == State::OPEN)
  {
    // In main thread
    WsMessage* msg = new (std::nothrow) WsMessage();
    msg->what = WS_MSG_TO_SUBTRHEAD_SENDING_BINARY;
    Data* data = new (std::nothrow) Data();
    data->bytes = new char[len];
    memcpy((void*)data->bytes, (void*)binaryMsg, len);
    data->len = len;
    msg->obj = data;
    _wsHelper->sendMessageToSubThread(msg);
  }
}

void WebSocket::close()
{
  if (_readyState == State::CLOSING || _readyState == State::CLOSED) {
    return;
  }
  
  webrtc::WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceUndefined, -1,
                      "websocket (%p) connection closed by client", this);
  _readyState = State::CLOSED;

  if (_wsHelper) _wsHelper->joinSubThread();
    
  // onClose callback needs to be invoked at the end of this method
  // since websocket instance may be deleted in 'onClose'.
  SignalCloseEvent(this);
}

WebSocket::State WebSocket::getReadyState()
{
  return _readyState;
}

int WebSocket::onSubThreadLoop()
{
  if (_readyState == State::CLOSED || _readyState == State::CLOSING) {
    libwebsocket_context_destroy(_wsContext);
    // return 1 to exit the loop.
    return 1;
  }
    
  if (_wsContext && _readyState != State::CLOSED && _readyState != State::CLOSING) {
    libwebsocket_service(_wsContext, 50);
  }
    
  // Sleep 50 ms
  //    std::this_thread::sleep_for(std::chrono::milliseconds(50));

  // return 0 to continue the loop.
  return 0;
}

void WebSocket::onSubThreadStarted()
{
  struct lws_context_creation_info info;
  memset(&info, 0, sizeof info);
    
  /*
  * create the websocket context.  This tracks open connections and
  * knows how to route any traffic and which protocol version to use,
  * and if each connection is client or server side.
  *
  * For this client-only demo, we tell it to not listen on any port.
  */

  info.port = CONTEXT_PORT_NO_LISTEN;
  info.protocols = _wsProtocols;
  #ifndef LWS_NO_EXTENSIONS
  info.extensions = libwebsocket_get_internal_extensions();
  #endif
  info.gid = -1;
  info.uid = -1;
  info.user = (void*)this;
    
  _wsContext = libwebsocket_create_context(&info);

  if(nullptr != _wsContext) {
    _readyState = State::CONNECTING;
    std::string name;
    for (int i = 0; _wsProtocols[i].callback != nullptr; ++i) {
      name += (_wsProtocols[i].name);

      if (_wsProtocols[i+1].callback != nullptr) name += ", ";
    }
    _wsInstance = libwebsocket_client_connect(_wsContext, _host.c_str(), _port, _SSLConnection,
                                              _path.c_str(), _host.c_str(), _host.c_str(),
                                              name.c_str(), -1);

    if(nullptr == _wsInstance) {
      _readyState = State::CLOSING;
      SignalErrorEvent(this, ErrorCode::UNKNOWN);
    }
  }
}

void WebSocket::onSubThreadEnded()
{

}

int WebSocket::onSocketCallback(struct libwebsocket_context *ctx,
                     struct libwebsocket *wsi,
                     int reason,
                     void *user, void *in, size_t len)
{
  webrtc::WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceUndefined, -1,
	                     "socket callback for %d reason", reason);
  ASSERT(_wsContext == nullptr || ctx == _wsContext); // Invalid context
  ASSERT(_wsInstance == nullptr || wsi == nullptr || wsi == _wsInstance); // Invaild websocket instance.

	switch (reason) {
    case LWS_CALLBACK_DEL_POLL_FD:
    case LWS_CALLBACK_PROTOCOL_DESTROY:
    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
      {
        WsMessage* msg = nullptr;
        if (reason == LWS_CALLBACK_CLIENT_CONNECTION_ERROR
            || (reason == LWS_CALLBACK_PROTOCOL_DESTROY && _readyState == State::CONNECTING)
            || (reason == LWS_CALLBACK_DEL_POLL_FD && _readyState == State::CONNECTING)
            ) {
          SignalErrorEvent(this, ErrorCode::UNKNOWN);
          _readyState = State::CLOSING;
        }
        else if (reason == LWS_CALLBACK_PROTOCOL_DESTROY && _readyState == State::CLOSING) {
          SignalCloseEvent(this);
        }
      }
      break;
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
      {
        _readyState = State::OPEN;
                
        /*
         * start the ball rolling,
         * LWS_CALLBACK_CLIENT_WRITEABLE will come next service
         */

        libwebsocket_callback_on_writable(ctx, wsi);
        SignalConnectEvent(this);
      }
      break;
            
    case LWS_CALLBACK_CLIENT_WRITEABLE:
      {
        std::lock_guard<std::mutex> lk(_wsHelper->_subThreadWsMessageQueueMutex);
                                               
        std::list<WsMessage*>::iterator iter = _wsHelper->_subThreadWsMessageQueue->begin();
                
        int bytesWrite = 0;
        for (; iter != _wsHelper->_subThreadWsMessageQueue->end();) {
          WsMessage* subThreadMsg = *iter;
                    
          if ( WS_MSG_TO_SUBTRHEAD_SENDING_STRING == subThreadMsg->what
                || WS_MSG_TO_SUBTRHEAD_SENDING_BINARY == subThreadMsg->what) {
            Data* data = (Data*)subThreadMsg->obj;

            const size_t c_bufferSize = WS_WRITE_BUFFER_SIZE;

            size_t remaining = data->len - data->issued;
            size_t n = std::min(remaining, c_bufferSize );
            
            //fixme: the log is not thread safe
            webrtc::WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceUndefined, -1,
                                 "[websocket:send] total: %d, sent: %d, remaining: %d, buffer size: %d"
                                 , static_cast<int>(data->len), static_cast<int>(data->issued)
                                 , static_cast<int>(remaining), static_cast<int>(n));

            unsigned char* buf = new unsigned char[LWS_SEND_BUFFER_PRE_PADDING + n + LWS_SEND_BUFFER_POST_PADDING];
            memcpy((char*)&buf[LWS_SEND_BUFFER_PRE_PADDING], data->bytes + data->issued, n);
                        
            int writeProtocol;
                        
            if (data->issued == 0) {
						  if (WS_MSG_TO_SUBTRHEAD_SENDING_STRING == subThreadMsg->what) {
                writeProtocol = LWS_WRITE_TEXT;
              }
						  else {
                writeProtocol = LWS_WRITE_BINARY;
							}

							// If we have more than 1 fragment
							if (data->len > c_bufferSize)
								writeProtocol |= LWS_WRITE_NO_FIN;
            } else {
              // we are in the middle of fragments
              writeProtocol = LWS_WRITE_CONTINUATION;
              // and if not in the last fragment
              if (remaining != n)
                writeProtocol |= LWS_WRITE_NO_FIN;
            }

            bytesWrite = libwebsocket_write(wsi,  &buf[LWS_SEND_BUFFER_PRE_PADDING], n, (libwebsocket_write_protocol)writeProtocol);
            
            //fixme: the log is not thread safe
            webrtc::WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceUndefined, -1,
                                  "[websocket:send] bytesWrite => %d", bytesWrite);

            // Buffer overrun?
            if (bytesWrite < 0) {
              break;
            }
            // Do we have another fragments to send?
            else if (remaining != n) {
              data->issued += n;
              break;
            }
            // Safely done!
            else {
              CC_SAFE_DELETE_ARRAY(data->bytes);
              CC_SAFE_DELETE(data);
              CC_SAFE_DELETE_ARRAY(buf);
              _wsHelper->_subThreadWsMessageQueue->erase(iter++);
              CC_SAFE_DELETE(subThreadMsg);
            }
          }
        }
                
        /* get notified as soon as we can write again */
                
        libwebsocket_callback_on_writable(ctx, wsi);
      }
      break;
            
    case LWS_CALLBACK_CLOSED:
      {
        //fixme: the log is not thread safe
        webrtc::WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceUndefined, -1,
                            "%s", "connection closing..");

        _wsHelper->quitSubThread();
                
        if (_readyState != State::CLOSED) {
          _readyState = State::CLOSED;
          SignalCloseEvent(this);
        }
      }
      break;
            
    case LWS_CALLBACK_CLIENT_RECEIVE:
      {
        if (in && len > 0) {
          // Accumulate the data (increasing the buffer as we go)
          if (_currentDataLen == 0) {
            _currentData = new char[len];
            memcpy (_currentData, in, len);
            _currentDataLen = len;
          }
          else {
            char *new_data = new char [_currentDataLen + len];
            memcpy (new_data, _currentData, _currentDataLen);
            memcpy (new_data + _currentDataLen, in, len);
            CC_SAFE_DELETE_ARRAY(_currentData);
            _currentData = new_data;
            _currentDataLen = _currentDataLen + len;
          }

          _pendingFrameDataLen = libwebsockets_remaining_packet_payload (wsi);

          if (_pendingFrameDataLen > 0) {
            webrtc::WEBRTC_TRACE(webrtc::kTraceInfo, webrtc::kTraceUndefined, -1,
                                "%ld bytes of pending data to receive, consider increasing the libwebsocket rx_buffer_size value."
                                , _pendingFrameDataLen);
          }
                    
          // If no more data pending, send it to the client thread
          if (_pendingFrameDataLen == 0) {
            char* bytes = nullptr;
            Data* data = new (std::nothrow) Data();

            if (lws_frame_is_binary(wsi)) {
              bytes = new char[_currentDataLen];
              data->isBinary = true;
            }
					  else {
              bytes = new char[_currentDataLen+1];
              bytes[_currentDataLen] = '\0';
              data->isBinary = false;
            }

            memcpy(bytes, _currentData, _currentDataLen);

            data->bytes = bytes;
            data->len = _currentDataLen;

            SignalReadEvent(this, *data);

            CC_SAFE_DELETE_ARRAY(_currentData);
            _currentData = nullptr;
            _currentDataLen = 0;

            CC_SAFE_DELETE_ARRAY(data->bytes);
            CC_SAFE_DELETE(data);
          }
        }
      }
      break;
      
    default:
      break;
	}
    
	return 0;
}

///////////////////////////////////////////////////////////////////////////////

} // namespace hotline

