#include "ban.hpp"
#include <gtest/gtest.h>

using namespace std::chrono_literals;
using clock_t_ = BanTracker::clock;

// Work well away from the steady_clock epoch so that subtracting the window
// never underflows and default-constructed time_points are unambiguous.
static const clock_t_::time_point kBase = clock_t_::time_point{} + 1000h;

TEST(BanTracker, UnknownIpIsNotBlocked) {
  BanTracker bt;
  EXPECT_FALSE(bt.is_blocked("1.2.3.4", kBase));
}

TEST(BanTracker, BlocksOnlyAfterMoreThanThreshold) {
  BanTracker bt; // default threshold = 3, so block on the 4th failure
  EXPECT_FALSE(bt.record_offense("1.2.3.4", kBase).blocked); // 1
  EXPECT_FALSE(bt.record_offense("1.2.3.4", kBase).blocked); // 2
  EXPECT_FALSE(bt.record_offense("1.2.3.4", kBase).blocked); // 3
  EXPECT_FALSE(bt.is_blocked("1.2.3.4", kBase));
  auto r = bt.record_offense("1.2.3.4", kBase); // 4
  EXPECT_TRUE(r.blocked);
  EXPECT_EQ(r.count, 4);
  EXPECT_TRUE(bt.is_blocked("1.2.3.4", kBase));
}

TEST(BanTracker, TracksEachIpIndependently) {
  BanTracker bt;
  for (int i = 0; i < 4; ++i) {
    bt.record_offense("1.1.1.1", kBase);
  }
  EXPECT_TRUE(bt.is_blocked("1.1.1.1", kBase));
  EXPECT_FALSE(bt.is_blocked("2.2.2.2", kBase));
}

TEST(BanTracker, OffensesAgeOutOfRollingWindow) {
  BanTracker bt;
  // Four failures spread over a couple of hours -> blocked.
  for (int i = 0; i < 4; ++i) {
    bt.record_offense("1.2.3.4", kBase + i * 1h);
  }
  EXPECT_TRUE(bt.is_blocked("1.2.3.4", kBase + 3h));

  // 24h after the first failure, that one drops out of the window: only 3
  // remain, so the IP is no longer blocked.
  EXPECT_FALSE(bt.is_blocked("1.2.3.4", kBase + 24h + 1min));
}

TEST(BanTracker, WindowBoundaryIsExclusiveAtCutoff) {
  BanTracker bt;
  // Exactly window-old timestamps are pruned (cutoff is inclusive of <=).
  bt.record_offense("1.2.3.4", kBase);
  auto r = bt.record_offense("1.2.3.4", kBase + 24h);
  EXPECT_EQ(r.count, 1); // the kBase entry was pruned before appending
}

TEST(BanTracker, SweepRemovesFullyExpiredIp) {
  BanTracker bt;
  for (int i = 0; i < 4; ++i) {
    bt.record_offense("1.2.3.4", kBase);
  }
  EXPECT_EQ(bt.tracked(), 1u);
  bt.sweep(kBase + 24h + 1min); // all offenses aged out
  EXPECT_EQ(bt.tracked(), 0u);
}

TEST(BanTracker, SweepKeepsStillActiveIp) {
  BanTracker bt;
  for (int i = 0; i < 4; ++i) {
    bt.record_offense("1.2.3.4", kBase);
  }
  bt.sweep(kBase + 1h); // still inside the window
  EXPECT_EQ(bt.tracked(), 1u);
  EXPECT_TRUE(bt.is_blocked("1.2.3.4", kBase + 1h));
}

TEST(BanTracker, RespectsCustomConfig) {
  BanTracker bt(BanTracker::Config{/*threshold=*/1, /*window=*/1h});
  EXPECT_FALSE(bt.record_offense("9.9.9.9", kBase).blocked); // 1, not > 1
  EXPECT_TRUE(bt.record_offense("9.9.9.9", kBase).blocked);  // 2 > 1
  EXPECT_TRUE(bt.is_blocked("9.9.9.9", kBase));
  EXPECT_FALSE(bt.is_blocked("9.9.9.9", kBase + 1h + 1min)); // window elapsed
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
