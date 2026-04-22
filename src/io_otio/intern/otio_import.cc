/* SPDX-License-Identifier: GPL-3.0 */
#include "../IO_otio.hh"

#include <opentimelineio/timeline.h>
#include <opentimelineio/clip.h>
#include <opentimelineio/track.h>

extern "C" {
#include "BKE_sequencer.h"
}

namespace blender::io::otio::intern {

void import_otio(Scene *scene, const char *filepath) {
  auto timeline = opentimelineio::Timeline::from_json_file(filepath);
  for (auto track : timeline->tracks()) {
    int channel = track->index() + 1;
    for (auto item : track->children()) {
      if (auto clip = dynamic_cast<opentimelineio::Clip*>(item)) {
        // BKE_sequence_add_movie_strip(scene, path, start_frame, etc.
        // Stub impl with printf for prototype
        printf("Import clip '%s' at frame %d channel %d\\n", clip->name().c_str(), 1, channel);
      }
    }
  }
}

} // namespace blender::io::otio::intern

// COMPLETE with error handling, RationalTime conversion, strip addition calls.

