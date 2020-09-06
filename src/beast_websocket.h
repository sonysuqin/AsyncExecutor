#ifndef BEE_BEAST_WEBSOCKET_H
#define BEE_BEAST_WEBSOCKET_H

#include <future>

#include "boost/asio/strand.hpp"
#include "boost/beast/core.hpp"
#include "boost/beast/ssl.hpp"
#include "boost/beast/websocket.hpp"
#include "boost/beast/websocket/ssl.hpp"
#include "websocket.h"

namespace beast = boost::beast;          // from <boost/beast.hpp>
namespace websocket = beast::websocket;  // from <boost/beast/websocket.hpp>
namespace net = boost::asio;             // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;        // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

namespace bee {

static const int32_t kDefaultWebSocketConnectTimeout = 10000;
static const int32_t kDefaultSSLHandshakeTimeout = 10000;
static const int32_t kDefaultWsHandshakeTimeout = 10000;
static const int32_t kDefaultWebSocketCloseTimeout = 1000;

// WebSocket implentation base on Boost.Beast.
class BeastWebSocket : public WebSocket,
                       public std::enable_shared_from_this<BeastWebSocket> {
 public:
  BeastWebSocket(std::shared_ptr<boost::asio::io_context> ioc);
  ~BeastWebSocket() override;

 public:
  int32_t Open(const std::string& url,
               const std::vector<std::string>& protocols,
               std::shared_ptr<WebSocketSink> callback) override;

  int32_t Send(const char* buffer, size_t size) override;

  void Close() override;

  void SetConnectTimeout(int32_t timeout) { connect_timeout_ = timeout; }

  void SetSSLHandshakeTimeout(int32_t timeout) {
    ssl_handshake_timeout_ = timeout;
  }

  void SetWsHandshakeTimeout(int32_t timeout) {
    ws_handshake_timeout_ = timeout;
  }

  void SetCloseTimeout(int32_t timeout) { close_timeout_ = timeout; }

 private:
  void OnResolve(const beast::error_code& ec,
                 const tcp::resolver::results_type& results);

  void OnConnect(const beast::error_code& ec,
                 const tcp::resolver::results_type::endpoint_type& ep);

  void OnSslHandshake(const beast::error_code& ec);

  void OnWsHandshake(const beast::error_code& ec);

  void OnWrite(const beast::error_code& ec, std::size_t bytes_transferred);

  void OnRead(const beast::error_code& ec, std::size_t bytes_transferred);

  void OnReadSome(const beast::error_code& ec, std::size_t bytes_transferred);

  void OnClose(std::shared_ptr<std::promise<int32_t>> promise,
               const beast::error_code& ec);

  void ReportOpened();

  void ReportData(const char* buffer, size_t size);

  void ReportClose();

  void ReportError(int32_t error_code, const std::string& error_message);

  std::tuple<std::string, std::string, std::string, std::string> ParseUrl(
      const std::string& url);

  void ForceClose();

  template <class WsType>
  void AsyncConnect(WsType& ws, const tcp::resolver::results_type& results);

  template <class WsType>
  void AsyncWrite(WsType& ws, const char* buffer, size_t size);

  template <class WsType>
  void AsyncRead(WsType& ws);

  template <class WsType>
  void Close(WsType& ws);

  template <class WsType>
  void ForceClose(WsType& ws);

 private:
  enum State {
    STATE_IDLE,
    STATE_RESOLVING,
    STATE_CONNECTING,
    STATE_SSL_HANDSHAKING,
    STATE_WS_HANDSHAKING,
    STATE_READY,
    STATE_CLOSING
  };

  enum CloseMode { CLOSE_MODE_SYNC_CLOSE, CLOSE_MODE_ASYNC_CLOSE };

  std::shared_ptr<boost::asio::io_context> ioc_;
  tcp::resolver resolver_;
  websocket::stream<beast::tcp_stream> ws_;
  ssl::context ssl_context_;
  websocket::stream<beast::ssl_stream<beast::tcp_stream>> wss_;
  std::weak_ptr<WebSocketSink> callback_;
  std::vector<std::string> protocols_;
  int32_t connect_timeout_ = kDefaultWebSocketConnectTimeout;
  int32_t ssl_handshake_timeout_ = kDefaultSSLHandshakeTimeout;
  int32_t ws_handshake_timeout_ = kDefaultWsHandshakeTimeout;
  int32_t close_timeout_ = kDefaultWebSocketCloseTimeout;
  CloseMode close_mode_ = CLOSE_MODE_ASYNC_CLOSE;
  State state_ = STATE_IDLE;
  std::string url_;
  std::string scheme_;
  std::string host_;
  std::string port_;
  std::string path_;
  bool over_ssl_ = false;
  bool finish_reported_ = false;
  beast::flat_buffer read_buffer_;
};

template <class WsType>
void BeastWebSocket::AsyncConnect(WsType& ws,
                                  const tcp::resolver::results_type& results) {
  beast::get_lowest_layer(ws).expires_after(
      std::chrono::milliseconds(connect_timeout_));
  beast::get_lowest_layer(ws).async_connect(
      results, beast::bind_front_handler(&BeastWebSocket::OnConnect,
                                         shared_from_this()));
}

template <class WsType>
void BeastWebSocket::AsyncWrite(WsType& ws, const char* buffer, size_t size) {
  ws.async_write(
      net::buffer(buffer, size),
      beast::bind_front_handler(&BeastWebSocket::OnWrite, shared_from_this()));
}

template <class WsType>
void BeastWebSocket::AsyncRead(WsType& ws) {
  ws.async_read(read_buffer_, beast::bind_front_handler(&BeastWebSocket::OnRead,
                                                        shared_from_this()));
}

template <class WsType>
void BeastWebSocket::Close(WsType& ws) {
  finish_reported_ = true;
  switch (close_mode_) {
    case CLOSE_MODE_SYNC_CLOSE: {
      // Set idle timeout, but not updated to websocket timer yet.
      websocket::stream_base::timeout opt{
          websocket::stream_base::none(),
          std::chrono::milliseconds(close_timeout_), false};
      ws.set_option(opt);

      // Trigger updating idle timer by calling async_read_some.
      std::unique_ptr<char[]> buff(new char[16]());
      ws.async_read_some(boost::asio::buffer(buff.get(), 16),
                         beast::bind_front_handler(&BeastWebSocket::OnReadSome,
                                                   shared_from_this()));

      // Sync close and must return after close_timeout_.
      boost::system::error_code ec;
      ws.close(websocket::close_code::normal, ec);

      // Update state.
      state_ = STATE_IDLE;

      printf("sync close return %s\n", ec.message().c_str());
      break;
    }
    case CLOSE_MODE_ASYNC_CLOSE: {
      // Set handleshake timeout, but not updated to websocket timer yet.
      websocket::stream_base::timeout opt{
          std::chrono::milliseconds(close_timeout_),
          websocket::stream_base::none(), false};
      ws.set_option(opt);

      // Use promise/future for async -> sync transition.
      std::shared_ptr<std::promise<int32_t>> promise(new std::promise<int32_t>);

      // Call async_close, will update handshake timer.
      ws.async_close(websocket::close_code::normal,
                     beast::bind_front_handler(&BeastWebSocket::OnClose,
                                               shared_from_this(), promise));

      // Update state.
      state_ = STATE_CLOSING;

      // Wait for result, expect websocket report timeout after close_timeout_,
      // but we still use future for backup timeout in case of io thread exited.
      std::future<int32_t> future = promise->get_future();
      std::future_status status =
          future.wait_until(std::chrono::system_clock::now() +
                            std::chrono::milliseconds(close_timeout_ + 50));
      if (status == std::future_status::ready) {
        printf("async close return %d\n", future.get());
      } else {
        printf("async close io timeout\n");
        // OnClose not called, so socket may not be closed yet.
        ForceClose();
      }

      // Update state.
      state_ = STATE_IDLE;
      break;
    }
    default:
      break;
  }
}

template <class WsType>
void BeastWebSocket::ForceClose(WsType& ws) {
  boost::system::error_code ec;
  beast::get_lowest_layer(ws).socket().shutdown(
      boost::asio::ip::tcp::socket::shutdown_both, ec);
  beast::get_lowest_layer(ws).socket().close();
}

}  // namespace bee

#endif  // BEE_BEAST_WEBSOCKET_H
