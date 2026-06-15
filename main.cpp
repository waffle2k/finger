#include <iostream>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/deferred.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/write.hpp>
#include <chrono>
#include <cstdio>

#include "ban.hpp"
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

awaitable<void> echo(tcp::socket socket, std::string client_addr, bool trackable,
                     BanTracker &bans) {
  try {
    auto now = std::chrono::steady_clock::now();

    // An IP that has racked up too many failed lookups (scanners, username
    // guessers, non-finger junk) is dropped without being read or answered.
    // Only globally-routable addresses are tracked: behind Docker's bridge
    // every client is SNAT'd to the gateway, so banning there would block
    // everyone at once (see is_bannable_address()).
    if (trackable && bans.is_blocked(client_addr, now)) {
      std::printf("finger drop from %s: blocked\n", client_addr.c_str());
      co_return;
    }

    char data[1024];
    auto [read_ec, bytes_read] = co_await socket.async_read_some(
        boost::asio::buffer(data), boost::asio::as_tuple(deferred));
    if (read_ec) {
      // Client hung up before sending a request: health checks (which connect
      // and immediately close), port scanners, and reset connections all land
      // here. This is normal -- don't log it as an exception.
      co_return;
    }
    std::string username(data, bytes_read);
    // Remove trailing \r\n characters
    while (!username.empty() &&
           (username.back() == '\r' || username.back() == '\n')) {
      username.pop_back();
    }
    std::printf("finger request from %s for user '%s'\n",
                client_addr.c_str(), username.c_str());
    auto response = co_await dofinger(username);

    // A "failure" is simply any request that does not resolve to a readable
    // plan file: an unknown user, rejected input, or non-finger junk. Each
    // failure is timestamped against the client IP; once an IP exceeds the
    // threshold within the rolling window, the is_blocked() check above starts
    // dropping its connections. This also frustrates username guessing.
    bool plan_served =
        response != username && response.rfind("InvalidInput:", 0) != 0;
    if (!plan_served) {
      if (trackable) {
        auto res = bans.record_offense(client_addr, now);
        std::printf("finger miss from %s for '%s' (%d failures in window)%s\n",
                    client_addr.c_str(), username.c_str(), res.count,
                    res.blocked ? " -- now blocked" : "");
      } else {
        std::printf("finger miss from %s for '%s' (not tracked)\n",
                    client_addr.c_str(), username.c_str());
      }
      // Best-effort reply; ignore write errors (the client may have already
      // gone away).
      co_await async_write(socket,
                           boost::asio::buffer(std::string("No plan found\r\n")),
                           boost::asio::as_tuple(deferred));
      co_return;
    }
    co_await async_write(socket, boost::asio::buffer(response),
                         boost::asio::as_tuple(deferred));
    co_return;
  } catch (std::exception &e) {
    std::printf("echo exception: %s\n", e.what());
  }
}

awaitable<void> listener(BanTracker &bans) {
  auto executor = co_await this_coro::executor;
  tcp::acceptor acceptor(executor, {tcp::v4(), 79});
  for (;;) {
    tcp::socket socket = co_await acceptor.async_accept(deferred);
    boost::system::error_code ec;
    auto endpoint = socket.remote_endpoint(ec);
    std::string client_addr =
        ec ? std::string("unknown") : endpoint.address().to_string();
    bool trackable = !ec && is_bannable_address(endpoint.address());
    co_spawn(executor,
             echo(std::move(socket), std::move(client_addr), trackable, bans),
             detached);
  }
}

// Periodically prune offense records that have aged out of the window so the
// tracker's memory stays bounded even for IPs that never reconnect.
awaitable<void> sweeper(BanTracker &bans) {
  boost::asio::steady_timer timer(co_await this_coro::executor);
  for (;;) {
    timer.expires_after(std::chrono::minutes(10));
    co_await timer.async_wait(deferred);
    bans.sweep(std::chrono::steady_clock::now());
  }
}

int main() {
  // Line-buffer stdout so docker logs / tail -f see entries in real time.
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  try {
    boost::asio::io_context io_context(1);
    BanTracker bans;

    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });

    co_spawn(io_context, listener(bans), detached);
    co_spawn(io_context, sweeper(bans), detached);

    io_context.run();
  } catch (std::exception &e) {
    std::printf("fatal exception: %s\n", e.what());
  }
}
