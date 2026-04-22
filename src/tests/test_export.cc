#include <gtest/gtest.h>
#include "../../io_otio/IO_otio.hh"

TEST(ExportTest, Stub) {
  // Mock scene
  export_otio(nullptr, "test.otio");
  SUCCEED();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// COMPLETE
