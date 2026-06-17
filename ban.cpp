#include "ban.hpp"

#include <cstdint>
#include <string>

bool is_bannable_address(const boost::asio::ip::address &addr) {
  if (addr.is_loopback() || addr.is_unspecified() || addr.is_multicast()) {
    return false;
  }

  if (addr.is_v4()) {
    const std::uint32_t a = addr.to_v4().to_uint();
    if ((a & 0xFF000000u) == 0x0A000000u) return false; // 10.0.0.0/8
    if ((a & 0xFFF00000u) == 0xAC100000u) return false; // 172.16.0.0/12
    if ((a & 0xFFFF0000u) == 0xC0A80000u) return false; // 192.168.0.0/16
    if ((a & 0xFFFF0000u) == 0xA9FE0000u) return false; // 169.254.0.0/16 link-local
    if ((a & 0xFFC00000u) == 0x64400000u) return false; // 100.64.0.0/10 CGNAT / Tailscale
    return true;
  }

  // IPv6: drop link-local (fe80::/10) and unique-local (fc00::/7).
  const auto v6 = addr.to_v6();
  if (v6.is_link_local()) {
    return false;
  }
  if ((v6.to_bytes()[0] & 0xFEu) == 0xFCu) {
    return false;
  }
  return true;
}

std::unordered_set<std::string> parse_ip_allowlist(std::string_view csv) {
  std::unordered_set<std::string> out;
  std::size_t start = 0;
  while (start <= csv.size()) {
    const std::size_t comma = csv.find(',', start);
    const std::size_t end =
        (comma == std::string_view::npos) ? csv.size() : comma;
    std::string_view tok = csv.substr(start, end - start);
    const std::size_t a = tok.find_first_not_of(" \t\r\n");
    if (a != std::string_view::npos) {
      const std::size_t b = tok.find_last_not_of(" \t\r\n");
      out.emplace(tok.substr(a, b - a + 1));
    }
    if (comma == std::string_view::npos) {
      break;
    }
    start = comma + 1;
  }
  return out;
}

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
