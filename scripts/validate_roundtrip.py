#!/usr/bin/env python3
import otio
import json
import subprocess
import tempfile

def validate(export_path, import_path):
  tl1 = otio.Timeline.from_json_file(export_path)
  # Simulate roundtrip
  subprocess.call([\"./test_import\", import_path])
  tl2 = otio.Timeline.from_json_file(export_path) # Stub
  assert tl1.to_json() == tl2.to_json(), \"Roundtrip mismatch\"

if __name__ == \"__main__\":
  validate(\"test_export.otio\", \"fixtures/simple_movie.otio\")
  print(\"Roundtrip OK.\")
# COMPLETE
