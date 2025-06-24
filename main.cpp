

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
  // co_return std::string("this is some finger information for ") + username +
  // std::string("\r\n");
}

awaitable<void> echo(tcp::socket socket) {
  try {
    char data[1024];
    auto bytes_read =
        co_await socket.async_read_some(boost::asio::buffer(data), deferred);
    // strip \r\n from data
    std::string username(data, bytes_read);
    // Remove trailing \r\n characters
    while (!username.empty() &&
           (username.back() == '\r' || username.back() == '\n')) {
      username.pop_back();
    }
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
    std::printf("echo Exception: %s\n", e.what());
  }
}

awaitable<void> listener() {
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), 79});
  for (;;) {
    tcp::socket socket = co_await acceptor.async_accept(deferred);
    co_spawn(executor, echo(std::move(socket)), detached);
  }
}

int main() {
  try {
    boost::asio::io_context io_context(1);

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });

    co_spawn(io_context, listener(), detached);

    io_context.run();
  } catch (std::exception &e) {
    std::printf("Exception: %s\n", e.what());
  }
}
