# blender-otio-gsoc2026

[![GSoC 2026](https://img.shields.io/badge/GSoC-2026-orange?logo=google)](https://summerofcode.withgoogle.com/)
[![Organization: Blender](https://img.shields.io/badge/org-Blender-blue?logo=blender)](https://www.blender.org/)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-green)](LICENSE)
[![OTIO](https://img.shields.io/badge/OpenTimelineIO-0.17-purple)](https://github.com/AcademySoftwareFoundation/OpenTimelineIO)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)

> **GSoC 2026 | Blender Foundation**  
> **Project:** VSE — OpenTimelineIO Import/Export  
> **Mentors:** TBD  

---

## What This Project Does

This repository tracks the design, research, and implementation work for adding **native OpenTimelineIO (OTIO) import/export support** to Blender's Video Sequence Editor (VSE) — as a built-in C++ feature, not an addon.

OTIO is an open interchange format for editorial timelines, developed by Pixar and now maintained by the Academy Software Foundation (ASWF). Adding OTIO support to Blender's VSE means:

- **Round-trip editing** between Blender and DaVinci Resolve, Premiere Pro, Final Cut Pro, Avid Media Composer
- **Pipeline integration** with OpenPype/Ayon, Kitsu, ShotGrid, and any tool using OTIO as its interchange layer
- **A first-class open-source implementation** in a major DCC application

---

## Repository Structure

```
blender-otio-gsoc2026/
│
├── PROPOSAL.md              ← Full GSoC proposal document
├── timeline.md              ← 350-hour weekly breakdown
├── design.md                ← Technical design doc (architecture, data model)
├── CONTRIBUTING.md          ← How to contribute to this research repo
│
├── src/                     ← Reference/prototype C++ implementation
│   ├── io_otio/
│   │   ├── CMakeLists.txt
│   │   ├── IO_otio.hh           ← Public API header
│   │   └── intern/
│   │       ├── otio_import.cc   ← OTIO → VSE import logic
│   │       ├── otio_export.cc   ← VSE → OTIO export logic
│   │       └── otio_utils.cc    ← Shared time/path utilities
│   └── tests/
│       ├── CMakeLists.txt
│       ├── test_timecode.cc
│       ├── test_import.cc
│       ├── test_export.cc
│       └── fixtures/            ← .otio test files
│           ├── simple_movie.otio
│           ├── multi_track.otio
│           ├── with_transitions.otio
│           ├── image_sequence.otio
│           ├── missing_media.otio
│           └── cross_fps_23976.otio
│
├── research/
│   ├── vse_io_audit.md          ← Analysis of the 2021 Python addon
│   ├── otio_api_notes.md        ← C++ API usage notes and gotchas
│   ├── blender_seq_internals.md ← Notes from reading sequencer source
│   └── comparable_implementations.md ← How Resolve/Premiere do it
│
├── scripts/
│   ├── build_test_env.sh        ← Builds Blender + OTIO from source
│   ├── generate_fixtures.py     ← Creates .otio test fixtures via Python API
│   └── validate_roundtrip.py    ← Checks export→import fidelity
│
└── .github/
    ├── ISSUE_TEMPLATE/
    │   ├── bug_report.md
    │   └── implementation_note.md
    └── PULL_REQUEST_TEMPLATE.md
```

---

## Technical Overview

### The Mapping Problem

```
OpenTimelineIO                    Blender VSE
─────────────────────────────────────────────────────────
Timeline                    ←→   Scene (sequencer context)
Stack                       ←→   All channels combined
Track (Video/Audio)         ←→   Channel (int, 1-indexed)
Clip + ExternalReference    ←→   MOVIE strip / IMAGE strip
Clip + (audio reference)    ←→   SOUND strip
Gap                         ←→   Empty space between strips
Transition (Dissolve)       ←→   CROSS effect strip
Transition (Wipe)           ←→   WIPE effect strip
Marker                      ←→   TimelineMarker
metadata dict               ←→   Custom properties (IDProperty)
RationalTime(value, rate)   ←→   Frame number (int) + scene FPS
```

### Architecture

```
Blender File Menu
       │
       ▼
SEQUENCER_OT_import_otio / SEQUENCER_OT_export_otio
(source/blender/editors/space_sequencer/sequencer_io_otio.cc)
       │
       ▼
bf_io_otio module
(source/blender/io/otio/intern/)
  ├── otio_import.cc   ← calls BKE_sequence_add_* functions
  ├── otio_export.cc   ← reads Sequence DNA, builds OTIO objects
  └── otio_utils.cc    ← RationalTime ↔ frame, path normalization
       │
       ▼
OpenTimelineIO C++ API
(extern/opentimelineio or system package)
```

---

## Building the Prototype

### Prerequisites
- CMake 3.21+
- C++17 compiler
- OpenTimelineIO 0.17+ (`pip install opentimelineio` also installs C++ headers on many platforms, or build from source)

### Build

```bash
git clone https://github.com/YOUR_HANDLE/blender-otio-gsoc2026.git
cd blender-otio-gsoc2026

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DOPENTIMELINEIO_ROOT=/usr/local
make -j$(nproc)
ctest --output-on-failure
```

---

## Test Fixtures

The `src/tests/fixtures/` directory contains hand-crafted `.otio` files covering:

| File | Tests |
|---|---|
| `simple_movie.otio` | Single video track, 3 clips, no gaps |
| `multi_track.otio` | 2 video + 1 audio track |
| `with_transitions.otio` | SMPTE_Dissolve between clips |
| `image_sequence.otio` | `ImageSequenceReference` |
| `missing_media.otio` | `MissingReference` handling |
| `cross_fps_23976.otio` | 23.976fps timeline into 24fps scene |
| `nested_stack.otio` | Nested composition (stretch goal) |

Generate fresh fixtures:
```bash
python scripts/generate_fixtures.py --output src/tests/fixtures/
```

---

## Related Resources

- [OpenTimelineIO GitHub](https://github.com/AcademySoftwareFoundation/OpenTimelineIO)
- [OTIO C++ API Docs](https://opentimelineio.readthedocs.io/en/latest/api/cpp/)
- [Blender VSE Documentation](https://docs.blender.org/manual/en/latest/video_editing/)
- [Blender Source: sequencer module](https://projects.blender.org/blender/blender/src/branch/main/source/blender/editors/space_sequencer)
- [vse_io addon (2021, Python)](https://projects.blender.org/blender-studio/scripts-blender)
- [ASWF OpenTimelineIO](https://www.aswf.io/projects/opentimelineio/)

---

## Prior Art Analysis

The 2021 `vse_io` Python addon (Blender Studio) demonstrated feasibility but had limitations:

| Feature | vse_io (2021) | This Project |
|---|---|---|
| Language | Python | C++ (native) |
| Movie clips | ✓ | ✓ |
| Audio clips | ✗ | ✓ |
| Image sequences | Partial | ✓ |
| Transitions | ✗ | ✓ |
| Markers | ✗ | ✓ |
| Metadata round-trip | ✗ | ✓ |
| Missing media | ✗ | ✓ |
| File menu integration | ✗ | ✓ |
| Python API (bpy.ops) | ✗ | ✓ |
| Blender 4.x compatible | ✗ | ✓ |

---

## Progress Log

| Date | Milestone |
|---|---|
| 2026-03-01 | Repository created, PROPOSAL.md draft v0.1 |
| 2026-03-15 | design.md v0.2 — mentor feedback incorporated |
| 2026-04-01 | Test fixtures created, prototype build system working |
| *GSoC starts* | ... |

---

## License

This research repository is under [GPL-3.0](LICENSE), consistent with Blender's license.  
The final implementation will be submitted to Blender's main repository under the same terms.  
OpenTimelineIO is Apache 2.0, which is GPL-compatible.

