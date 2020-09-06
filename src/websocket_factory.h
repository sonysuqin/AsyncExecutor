#ifndef BEE_WEBSOCKET_FACTORY_H
#define BEE_WEBSOCKET_FACTORY_H

#include <memory>

#include "websocket.h"

namespace bee {

class WebSocketFactory {
 public:
  virtual std::shared_ptr<WebSocket> CreateWebSocket() = 0;
};

}  // namespace bee

#endif  // BEE_WEBSOCKET_FACTORY_H
