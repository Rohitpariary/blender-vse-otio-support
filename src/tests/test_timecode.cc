#include <gtest/gtest.h>
#include "../../io_otio/intern/otio_utils.cc" // Stub include

TEST(TimecodeTest, RationalToFrame) {
  EXPECT_EQ(rational_to_frame(opentimelineio::RationalTime(1, 24), 24.0), 1);
  EXPECT_EQ(rational_to_frame(opentimelineio::RationalTime(1001, 24000), 23.976), 1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// COMPLETE with more tests
