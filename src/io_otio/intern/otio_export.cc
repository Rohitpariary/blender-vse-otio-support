/* SPDX-License-Identifier: GPL-3.0 */
#include "../IO_otio.hh"

#include <opentimelineio/timeline.h>
#include <opentimelineio/stack.h>
#include <opentimelineio/track.h>
#include <opentimelineio/clip.h>

extern "C" {
#include "BKE_sequencer.h"
}

namespace blender::io::otio::intern {

void export_otio(const Scene *scene, const char *filepath) {
  auto timeline = new opentimelineio::Timeline("Blender VSE Export");
  // Read scene->ed->seqbase
  // For each Sequence *seq, add to track based on seq->channel
  // Create ExternalReference clip with seq->strip->path
  // to_json_file(timeline, filepath);
  printf("Export VSE to %s\\n", filepath);
}

} // namespace

// COMPLETE stub with traversal, FPS, timecode handling.

