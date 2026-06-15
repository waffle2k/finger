#include "ban.hpp"

namespace {
// Count timestamps that fall within (now - window, now]. The deque is kept in
// ascending order, so the in-window entries are always a suffix.
int count_in_window(const std::deque<BanTracker::clock::time_point> &ts,
                    BanTracker::clock::time_point now,
                    BanTracker::clock::duration window) {
  const auto cutoff = now - window;
  int count = 0;
  for (auto it = ts.rbegin(); it != ts.rend() && *it > cutoff; ++it) {
    ++count;
  }
  return count;
}
} // namespace

bool BanTracker::is_blocked(const std::string &ip, clock::time_point now) const {
  auto it = offenders_.find(ip);
  if (it == offenders_.end()) {
    return false;
  }
  return count_in_window(it->second, now, cfg_.window) > cfg_.threshold;
}

BanTracker::OffenseResult
BanTracker::record_offense(const std::string &ip, clock::time_point now) {
  auto &ts = offenders_[ip];
  const auto cutoff = now - cfg_.window;

  // Drop this IP's timestamps that have aged out of the window.
  while (!ts.empty() && ts.front() <= cutoff) {
    ts.pop_front();
  }

  ts.push_back(now);

  const int count = static_cast<int>(ts.size());
  return {count, count > cfg_.threshold};
}

void BanTracker::sweep(clock::time_point now) {
  const auto cutoff = now - cfg_.window;
  for (auto it = offenders_.begin(); it != offenders_.end();) {
    auto &ts = it->second;
    while (!ts.empty() && ts.front() <= cutoff) {
      ts.pop_front();
    }
    if (ts.empty()) {
      it = offenders_.erase(it);
    } else {
      ++it;
    }
  }
}
