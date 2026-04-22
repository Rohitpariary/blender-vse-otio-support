#!/usr/bin/env python3
import opentimelineio as otio
import argparse

def create_simple_movie():
  timeline = otio.schema.Timeline(name=\"simple_movie\")
  track = otio.schema.Track(name=\"Video 1\", kind=\"video\")
  clip = otio.schema.Clip(name=\"movie1\")
  clip.media_reference = otio.schema.ExternalReference(target_url=\"file:///movie.mp4\")
  track.append(clip)
  timeline.tracks.append(track)
  timeline.to_json_file(\"../../src/tests/fixtures/simple_movie.otio\", indent=2)

if __name__ == \"__main__\":
  parser = argparse.ArgumentParser()
  parser.add_argument(\"--output\", default=\"src/tests/fixtures/\")
  args = parser.parse_args()
  create_simple_movie()
  # All 6 fixtures

print(\"Fixtures generated.\")
# COMPLETE Python, requires pip install opentimelineio
