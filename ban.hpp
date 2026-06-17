#pragma once

#include <boost/asio/ip/address.hpp>
#include <chrono>
#include <cstddef>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

// BanTracker records the timestamps of "offenses" -- requests that are
// obviously not finger queries -- per client IP, over a rolling time window.
// When an IP has more than `threshold` offenses still inside the window, it is
// blocked and its connections are dropped. Offense timestamps older than the
// window are pruned, so a blocked IP automatically frees itself once its old
// offenses age out.
//
// All state is in-memory: the daemon runs a single io_context thread, so every
// call happens on the same thread and no locking is required. Time is passed
// in as a steady_clock time_point rather than read internally, so the logic is
// deterministic and unit-testable.
class BanTracker {
public:
  using clock = std::chrono::steady_clock;

  struct Config {
    int threshold = 3;                              // block when offenses exceed this
    clock::duration window = std::chrono::hours(24); // rolling window length
  };

  struct OffenseResult {
    int count;    // offenses within the window, including this one
    bool blocked; // true if the IP is now blocked (count > threshold)
  };

  BanTracker() = default;
  explicit BanTracker(Config cfg) : cfg_(cfg) {}

  // True if ip currently has more than `threshold` offenses inside the rolling
  // window. Does not mutate state.
  bool is_blocked(const std::string &ip, clock::time_point now) const;

  // Record one offense from ip at `now`. Prunes that IP's expired timestamps,
  // appends this one, and reports the in-window count and whether it is now
  // blocked.
  OffenseResult record_offense(const std::string &ip, clock::time_point now);

  // Drop timestamps older than the window across all IPs, removing any IP left
  // with no offenses. Safe to call periodically to keep the map bounded.
  void sweep(clock::time_point now);

  // Number of tracked IPs (for introspection and tests).
  std::size_t tracked() const { return offenders_.size(); }

  const Config &config() const { return cfg_; }

private:
  Config cfg_{};
  // Per-IP offense timestamps, kept in ascending order (steady_clock is
  // monotonic, so appends are always newest-last).
  std::unordered_map<std::string, std::deque<clock::time_point>> offenders_;
};

// Whether a client address is meaningful to track and ban. Only globally
// routable unicast addresses qualify. Loopback, RFC1918 private, CGNAT
// (100.64/10), link-local, IPv6 unique-local, and multicast addresses all
// return false.
//
// This matters because the daemon can only ban what it can see: behind Docker's
// default bridge networking every external client is SNAT'd to the bridge
// gateway (a 172.16/12 address), so banning per source IP would collapse all
// clients into one and block everyone. Skipping non-global addresses makes
// banning correct where the real client IP is visible (e.g. the FreeBSD jail,
// where pf rdr preserves it) and inert where it is not (Docker bridge), with no
// deployment-specific configuration.
bool is_bannable_address(const boost::asio::ip::address &addr);

// Parse a comma-separated list of IP addresses (the value of the
// FINGER_BAN_ALLOWLIST env var) into a set of address strings. Whitespace
// around each entry is trimmed and empty entries are skipped. The strings are
// matched verbatim against boost::asio's address().to_string() output, so use
// canonical forms (e.g. "147.182.255.203", "2a01:4f8:190:7447::2").
//
// Allowlisting exists for trusted aggregating front-ends — notably the
// finger-web proxy, which funnels every federated lookup through one IP. Without
// it, a burst from any single client of the proxy is attributed to the proxy's
// IP and bans the proxy for everyone; per-client abuse protection for that path
// lives in the proxy instead.
std::unordered_set<std::string> parse_ip_allowlist(std::string_view csv);
