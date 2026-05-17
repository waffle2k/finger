#include <iostream>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/write.hpp>
#include <cstdio>

#include "handler.hpp"

using boost::asio::awaitable;
using boost::asio::co_spawn;
using boost::asio::deferred;
using boost::asio::detached;
using boost::asio::ip::tcp;
namespace this_coro = boost::asio::this_coro;

awaitable<std::string> dofinger(const std::string &username) {
  co_return process(username);
}

awaitable<void> echo(tcp::socket socket, std::string client_addr) {
  try {
    char data[1024];
    auto bytes_read =
        co_await socket.async_read_some(boost::asio::buffer(data), deferred);
    std::string username(data, bytes_read);
    // Remove trailing \r\n characters
    while (!username.empty() &&
           (username.back() == '\r' || username.back() == '\n')) {
      username.pop_back();
    }
    std::printf("finger request from %s for user '%s'\n",
                client_addr.c_str(), username.c_str());
    auto response = co_await dofinger(username);
    if (response.compare(std::string(username)) == 0) {
      // No plan found
      co_await async_write(
          socket, boost::asio::buffer(std::string("No plan found\r\n")),
          deferred);
      co_return;
    }
    co_await async_write(socket, boost::asio::buffer(response), deferred);
    co_return;
  } catch (std::exception &e) {
    std::printf("echo exception: %s\n", e.what());
  }
}

awaitable<void> listener() {
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), 79});
  for (;;) {
    tcp::socket socket = co_await acceptor.async_accept(deferred);
    boost::system::error_code ec;
    auto endpoint = socket.remote_endpoint(ec);
    std::string client_addr =
        ec ? std::string("unknown") : endpoint.address().to_string();
    co_spawn(executor, echo(std::move(socket), std::move(client_addr)),
             detached);
  }
}

int main() {
  // Line-buffer stdout so docker logs / tail -f see entries in real time.
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  try {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  } catch (std::exception &e) {
    std::printf("fatal exception: %s\n", e.what());
  }
}
