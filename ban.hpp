#pragma once

#include <chrono>
#include <cstddef>
#include <deque>
#include <string>
#include <unordered_map>

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
