#ifndef BEE_WEBSOCKET_H
#define BEE_WEBSOCKET_H

#include <stdint.h>
#include <string>
#include <vector>

namespace bee {

class WebSocketSink {
 public:
  virtual void OnOpen() = 0;

  virtual void OnData(const char* buffer, size_t size) = 0;

  virtual void OnClose() = 0;

  virtual void OnError(int32_t error_code,
                       const std::string& error_message) = 0;
};

class WebSocket {
 public:
  WebSocket() = default;
  virtual ~WebSocket() = default;

  virtual int32_t Open(const std::string& url,
                       const std::vector<std::string>& protocols,
                       std::shared_ptr<WebSocketSink> callback) = 0;

  virtual int32_t Send(const char* buffer, size_t size) = 0;

  virtual void Close() = 0;
};

}  // namespace bee

#endif  // BEE_WEBSOCKET_H
