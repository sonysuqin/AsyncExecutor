#include "beast_websocket.h"
#include "bee_define.h"
#include "timer.h"

namespace bee {

BeastWebSocket::BeastWebSocket(std::shared_ptr<boost::asio::io_context> ioc)
    : ioc_(ioc),
      resolver_(net::make_strand(*ioc)),
      ws_(net::make_strand(*ioc)),
      ssl_context_({ssl::context::sslv23_client}),
      wss_(net::make_strand(*ioc), ssl_context_) {
  printf("BeastWebSocket created\n");
}

BeastWebSocket::~BeastWebSocket() {
  printf("BeastWebSocket deleted\n");
}

int32_t BeastWebSocket::Open(const std::string& url,
                             const std::vector<std::string>& protocols,
                             std::shared_ptr<WebSocketSink> callback) {
  if (url.empty()) {
    return kBeeErrorCode_Invalid_Param;
  }

  if (state_ != STATE_IDLE) {
    return kBeeErrorCode_Invalid_State;
  }

  std::string scheme, host, port, path;
  std::tie(scheme, host, port, path) = ParseUrl(url);
  if (scheme.empty() || host.empty() || port.empty() || path.empty()) {
    return kBeeErrorCode_Invalid_Param;
  }

  url_ = url;
  protocols_ = protocols;
  callback_ = callback;
  scheme_ = scheme;
  host_ = host;
  port_ = port;
  path_ = path;
  over_ssl_ = (scheme == "wss");

  printf("Ws Open url:%s schme:%s host:%s port:%s path:%s ssl:%s\n",
         url_.c_str(), scheme_.c_str(), host_.c_str(), port_.c_str(),
         path_.c_str(), over_ssl_ ? "true" : "false");

  resolver_.async_resolve(host, port,
                          beast::bind_front_handler(&BeastWebSocket::OnResolve,
                                                    shared_from_this()));

  state_ = STATE_RESOLVING;
  return kBeeErrorCode_Success;
}

int32_t BeastWebSocket::Send(const char* buffer, size_t size) {
  if (state_ != STATE_READY) {
    return kBeeErrorCode_Invalid_State;
  }

  if (!over_ssl_) {
    AsyncWrite(ws_, buffer, size);
  } else {
    AsyncWrite(wss_, buffer, size);
  }

  return kBeeErrorCode_Success;
}

void BeastWebSocket::Close() {
  if (!over_ssl_) {
    Close(ws_);
  } else {
    Close(wss_);
  }
}

void BeastWebSocket::OnResolve(const beast::error_code& ec,
                               const tcp::resolver::results_type& results) {
  if (state_ != STATE_RESOLVING) {
    return;
  }

  if (ec) {
    ReportError(kBeeErrorCode_Resolve_Fail, ec.message());
    state_ = STATE_IDLE;
    printf("OnResolve error %s\n", ec.message().c_str());
    return;
  }

  for (auto iter = results.begin(); iter != results.end(); ++iter) {
    printf("Resolved %s:%d\n", iter->endpoint().address().to_string().c_str(),
           iter->endpoint().port());
  }

  if (!over_ssl_) {
    AsyncConnect(ws_, results);
  } else {
    AsyncConnect(wss_, results);
  }

  state_ = STATE_CONNECTING;
}

void BeastWebSocket::OnConnect(
    const beast::error_code& ec,
    const tcp::resolver::results_type::endpoint_type& ep) {
  if (state_ != STATE_CONNECTING) {
    return;
  }

  if (ec) {
    ReportError(kBeeErrorCode_Connect_Fail, ec.message());
    state_ = STATE_IDLE;
    printf("OnConnect error %s\n", ec.message().c_str());
    return;
  }

  printf("Connected to %s:%d\n", ep.address().to_string().c_str(), ep.port());

  if (!over_ssl_) {
    beast::get_lowest_layer(ws_).expires_never();
    websocket::stream_base::timeout opt{
        std::chrono::milliseconds(ws_handshake_timeout_),
        websocket::stream_base::none(), false};
    ws_.set_option(opt);
    ws_.async_handshake(
        host_, path_,
        beast::bind_front_handler(&BeastWebSocket::OnWsHandshake,
                                  shared_from_this()));
    state_ = STATE_WS_HANDSHAKING;
  } else {
    beast::get_lowest_layer(ws_).expires_after(
        std::chrono::milliseconds(ssl_handshake_timeout_));
    wss_.next_layer().async_handshake(
        ssl::stream_base::client,
        beast::bind_front_handler(&BeastWebSocket::OnSslHandshake,
                                  shared_from_this()));
    state_ = STATE_SSL_HANDSHAKING;
  }
}

void BeastWebSocket::OnSslHandshake(const beast::error_code& ec) {
  if (state_ != STATE_SSL_HANDSHAKING) {
    return;
  }

  if (ec) {
    ReportError(kBeeErrorCode_SSL_Handshake_Fail, ec.message());
    state_ = STATE_IDLE;
    printf("OnSslHandshake error %s\n", ec.message().c_str());
    return;
  }

  printf("SSL handshake success.\n");

  beast::get_lowest_layer(wss_).expires_never();
  websocket::stream_base::timeout opt{
      std::chrono::milliseconds(ws_handshake_timeout_),
      websocket::stream_base::none(), false};
  wss_.set_option(opt);
  wss_.async_handshake(host_, path_,
                       beast::bind_front_handler(&BeastWebSocket::OnWsHandshake,
                                                 shared_from_this()));
  state_ = STATE_WS_HANDSHAKING;
}

void BeastWebSocket::OnWsHandshake(const beast::error_code& ec) {
  if (state_ != STATE_WS_HANDSHAKING) {
    return;
  }

  if (ec) {
    printf("OnWsHandshake error %s\n", ec.message().c_str());
    state_ = STATE_IDLE;
    ReportError(kBeeErrorCode_Ws_Handshake_Fail, ec.message());
  } else {
    printf("Ws handshake success.\n");
    state_ = STATE_READY;
    ReportOpened();
    if (!over_ssl_) {
      AsyncRead(ws_);
    } else {
      AsyncRead(wss_);
    }
  }
}

void BeastWebSocket::OnWrite(const beast::error_code& ec,
                             std::size_t bytes_transferred) {
  if (ec) {
    ReportError(kBeeErrorCode_Write_Fail, ec.message());
    state_ = STATE_IDLE;
    return;
  }

  printf("Write %u bytes\n", bytes_transferred);
}

void BeastWebSocket::OnRead(const beast::error_code& ec,
                            std::size_t bytes_transferred) {
  if (ec) {
    ReportError(kBeeErrorCode_Read_Fail, ec.message());
    state_ = STATE_IDLE;
    return;
  }

  printf("Read %u bytes\n", bytes_transferred);
  printf("Read:%s\n", beast::buffers_to_string(read_buffer_.data()).c_str());

  if (!over_ssl_) {
    AsyncRead(ws_);
  } else {
    AsyncRead(wss_);
  }
}

void BeastWebSocket::OnReadSome(const beast::error_code& ec,
                                std::size_t bytes_transferred) {}

void BeastWebSocket::OnClose(std::shared_ptr<std::promise<int32_t>> promise,
                             const beast::error_code& ec) {
  printf("OnClose %d %s\n", ec.value(), ec.message().c_str());
  if (state_ != STATE_CLOSING) {
    printf("Warning, not in closing state.\n");
  }

  if (promise != nullptr) {
    promise->set_value(ec.value());
  }
}

void BeastWebSocket::ReportOpened() {
  std::shared_ptr<WebSocketSink> callback = callback_.lock();
  if (callback != nullptr) {
    callback->OnOpen();
  }
}

void BeastWebSocket::ReportData(const char* buffer, size_t size) {
  std::shared_ptr<WebSocketSink> callback = callback_.lock();
  if (callback != nullptr) {
    callback->OnData(buffer, size);
  }
}

void BeastWebSocket::ReportClose() {
  std::shared_ptr<WebSocketSink> callback = callback_.lock();
  if (callback != nullptr) {
    callback->OnClose();
    finish_reported_ = true;
  }
}

void BeastWebSocket::ReportError(int32_t error_code,
                                 const std::string& error_message) {
  std::shared_ptr<WebSocketSink> callback = callback_.lock();
  if (callback != nullptr && !finish_reported_) {
    callback->OnError(error_code, error_message);
    finish_reported_ = true;
  }
}

std::tuple<std::string, std::string, std::string, std::string>
BeastWebSocket::ParseUrl(const std::string& url) {
  std::string scheme("ws");
  auto scheme_pos_end = url.find("://");
  if (scheme_pos_end != std::string::npos) {
    scheme = url.substr(0, scheme_pos_end);
  }

  std::string host_str("");
  std::string port_str = (scheme == "wss") ? "443" : "80";
  std::string path_str("");

  auto host_pos_begin =
      (scheme_pos_end == std::string::npos ? 0 : scheme_pos_end + 3);
  auto host_pos_end = url.find_first_of(':', host_pos_begin);
  auto slash_pos = url.find_first_of('/', host_pos_begin);
  auto path_pos_begin = std::string::npos;

  // Full format "ws://x.x.x.x:xx/xxx"
  if (host_pos_end != std::string::npos && slash_pos != std::string::npos) {
    if (host_pos_end >= slash_pos) {
      return std::make_tuple(scheme, "", "", "");  // Invalid format.
    }
    host_str = url.substr(host_pos_begin, host_pos_end - host_pos_begin);
    auto port_pos_begin = host_pos_end + 1;
    auto port_pos_end = url.find_first_of('/', port_pos_begin);
    port_str = url.substr(port_pos_begin, port_pos_end - port_pos_begin);
    path_pos_begin = port_pos_end;
  } else {
    // Partial format "ws://x.x.x.x:xx" or "ws://x.x.x.x/xxx" or "ws://x.x.x.x"
    host_pos_end = url.find_first_of('/', host_pos_begin);
    if (host_pos_end != std::string::npos) {
      host_str = url.substr(host_pos_begin, host_pos_end - host_pos_begin);
      path_pos_begin = host_pos_end;
    } else {
      host_pos_end = url.find_first_of(':', host_pos_begin);
      if (host_pos_end != std::string::npos) {
        host_str = url.substr(host_pos_begin, host_pos_end - host_pos_begin);
        port_str = url.substr(host_pos_end + 1);
      } else {
        host_str = url.substr(host_pos_begin);
      }
    }
  }

  if (path_pos_begin != std::string::npos)
    path_str = url.substr(path_pos_begin);

  if (path_str.empty())
    path_str = "/";

  return std::make_tuple(scheme, host_str, port_str, path_str);
}

void BeastWebSocket::ForceClose() {
  if (!over_ssl_) {
    ForceClose(ws_);
  } else {
    ForceClose(wss_);
  }
}

}  // namespace bee
