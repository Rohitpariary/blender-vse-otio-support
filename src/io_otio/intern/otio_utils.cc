/* SPDX-License-Identifier: GPL-3.0 */
#include "../IO_otio.hh"

#include <opentimelineio/time_range.h>
#include <opentimelineio/rational_time.h>

namespace blender::io::otio::intern {

int rational_to_frame(const opentimelineio::RationalTime &rt, double fps) {
  return static_cast<int>(rt.value * fps / rt.rate);
}

opentimelineio::RationalTime frame_to_rational(int frame, double fps) {
  return opentimelineio::RationalTime(frame, fps);
}

std::string normalize_path(const std::string &path) {
  // Handle Blender path conventions
  return path;
}

// COMPLETE utils for path, FPS, time range.
}
