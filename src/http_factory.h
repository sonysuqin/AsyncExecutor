#ifndef BEE_HTTP_FACTORY_H
#define BEE_HTTP_FACTORY_H

#include <memory>

#include "http.h"

namespace bee {

class HttpFactory {
 public:
  virtual std::shared_ptr<Http> CreateHttp() = 0;
};

}  // namespace bee

#endif  // BEE_HTTP_FACTORY_H
