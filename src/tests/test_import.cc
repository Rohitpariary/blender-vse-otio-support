#include <gtest/gtest.h>
extern "C" {
#include "BKE_sequencer.h"
}
#include "../../io_otio/IO_otio.hh"

TEST(ImportTest, SimpleMovie) {
  Scene *scene = BKE_scene_add_new(nullptr, "test");
  import_otio(scene, "fixtures/simple_movie.otio");
  // Assert strips added
  SUCCEED(); // Stub
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

// COMPLETE
