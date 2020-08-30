#ifndef __WEBSOCKET_FACTORY_H__
#define __WEBSOCKET_FACTORY_H__

#include <memory>

#include "websocket.h"

namespace bee {

class WebSocketFactory {
 public:
  virtual std::shared_ptr<WebSocket> CreateWebSocket() = 0;
};

}  // namespace bee

#endif  // __WEBSOCKET_FACTORY_H__
