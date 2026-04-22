# GSoC 2026 Proposal: VSE — OpenTimelineIO Support

**Organization:** Blender Foundation  
**Project:** VSE: OpenTimelineIO Import/Export  
**Applicant:** [Rohit Pariary]  
**Email:** [rohit.pariary2002@gmail.com]  
**GitHub:** [github.com/Rohitpariary]  
**Timezone:** [ Kolkata, West Bengal (GMT+5:30)]  
**Availability:** 40 hours/week (350 hours total)

---

## Abstract

Blender's Video Sequence Editor (VSE) is a fully-featured non-linear video editor embedded in a 3D content creation suite — an unusual and powerful combination. However, its isolation from the broader post-production ecosystem remains a real pain point: studios cannot reliably exchange edit decisions with tools like DaVinci Resolve, Adobe Premiere, or Final Cut Pro without lossy workarounds. OpenTimelineIO (OTIO), a Pixar-originated open standard now under the ASWF umbrella, was designed specifically to solve this problem.

This project delivers **native, built-in OTIO import/export support inside Blender's VSE** — not as an addon, but as a first-class feature integrated into Blender's file I/O pipeline. The implementation covers the full OTIO schema: Timelines, Tracks, Clips (image, sound, movie), Transitions, Effects, Markers, and metadata — with bidirectional fidelity. The architecture is designed to be maintainable, extensible, and production-ready from day one.

---

## 1. Benefits to Blender and the Open-Source Community

### For Blender Studios and Professional Users
- **Round-trip editing:** Export a cut from Blender to DaVinci Resolve for color grading, then re-import the conformed timeline back.
- **Pipeline integration:** Studios using OpenPype/Ayon, Kitsu, or ShotGrid can now include Blender VSE in production pipelines that depend on OTIO as the interchange format.
- **EDL/AAF replacement:** OTIO supersedes fragile EDL and AAF workflows, which currently require third-party tools when working with Blender-originated timelines.

### For the Open-Source Ecosystem
- **ASWF collaboration:** Aligns Blender with the Academy Software Foundation ecosystem — the same layer used by OpenColorIO, OpenEXR, OpenVDB, and MaterialX, all of which Blender already integrates.
- **Reference implementation:** Blender's OTIO integration will become one of the most complete open-source implementations, serving as a reference for other DCC tools.
- **Blender Studio workflows:** Blender Studio (the films division) has already experimented with this via `vse_io` (2021). A built-in solution removes their dependency on an unmaintained addon.

---

## 2. Problem Statement

### Current State
Blender's VSE has no native mechanism to read or write industry-standard timeline interchange formats. The only available path is:

```
Blender VSE → (manual) → EDL (CMX 3600) → third-party converter → target NLE
```

This chain is fragile, lossy, and unsupported. The `vse_io` addon (blender-studio/scripts-blender, 2021) demonstrated OTIO feasibility in Python but is:
- No longer actively maintained
- Limited to simple clip/track mapping (no transitions, no nested timelines)
- Not accessible from Blender's standard File > Import/Export menu
- Incompatible with Blender 4.x sequencer internals

### What OTIO Solves
OTIO provides a fully-specified, schema-versioned, language-agnostic representation of non-linear timelines. It supports:
- Multiple video/audio tracks with precise rational-number timecodes
- Gaps, clips, transitions (dissolves, wipes)
- Nested compositions (stacks within tracks)
- Metadata dictionaries for arbitrary per-item data
- Media references with file paths, URL schemes, and missing-media handling

### The Gap
No DCC application in the open-source space has a **native, compiled, built-in** OTIO integration. This project fills that gap.

---

## 3. Technical Approach

### 3.1 Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     BLENDER APPLICATION                      │
│                                                             │
│  ┌──────────────┐    ┌─────────────────────────────────┐   │
│  │   File Menu  │    │     VSE (sequencer module)       │   │
│  │  Import/Export│───▶│  bke_sequencer / seq_edit.cc   │   │
│  └──────────────┘    └────────────┬────────────────────┘   │
│                                   │                          │
│              ┌────────────────────┼────────────────────┐    │
│              ▼                    ▼                    ▼     │
│   ┌──────────────────┐  ┌────────────────┐  ┌─────────────┐│
│   │  io_otio_import  │  │ io_otio_export │  │ otio_utils  ││
│   │  (C++ operator)  │  │ (C++ operator) │  │  (mapping)  ││
│   └────────┬─────────┘  └───────┬────────┘  └──────┬──────┘│
│            │                    │                   │       │
└────────────┼────────────────────┼───────────────────┼───────┘
             │                    │                   │
             ▼                    ▼                   │
     ┌───────────────────────────────────┐            │
     │       OpenTimelineIO C++ API      │◀───────────┘
     │   opentimelineio::v1_0::Timeline  │
     │   opentimelineio::v1_0::Track     │
     │   opentimelineio::v1_0::Clip      │
     │   opentimelineio::v1_0::Transition│
     └───────────────────────────────────┘
```

### 3.2 Blender Source Code Integration Points

The implementation touches four areas of Blender's source tree:

```
blender/
├── source/blender/
│   ├── editors/space_sequencer/
│   │   └── sequencer_io_otio.cc        ← NEW: operator + UI
│   ├── blenkernel/
│   │   ├── BKE_sequencer_otio.hh       ← NEW: public API declarations
│   │   └── intern/
│   │       └── sequencer_otio.cc       ← NEW: core conversion logic
│   └── io/
│       └── otio/
│           ├── CMakeLists.txt          ← NEW
│           ├── intern/
│           │   ├── otio_import.cc      ← NEW: OTIO→VSE
│           │   ├── otio_export.cc      ← NEW: VSE→OTIO
│           │   └── otio_utils.cc       ← NEW: shared helpers
│           └── IO_otio.hh              ← NEW: public header
├── extern/
│   └── opentimelineio/                 ← NEW: vendored or find_package
│       └── CMakeLists.txt
└── build_files/cmake/
    └── platform/
        └── platform_unix.cmake         ← MODIFIED: add OTIO find
```

### 3.3 OTIO Schema → Blender VSE Mapping

| OTIO Object | Blender VSE Equivalent | Notes |
|---|---|---|
| `Timeline` | `Scene` (sequencer context) | Top-level container |
| `Stack` | All tracks combined | VSE has flat track model |
| `Track` (video) | `SEQ_CHAN_*` channel | Channel number = track index |
| `Track` (audio) | Audio channel | Separated by track kind |
| `Clip` (movie) | `MOVIE` strip (`MovieSequence`) | File path + in/out points |
| `Clip` (image seq) | `IMAGE` strip (`ImageSequence`) | Frame range |
| `Clip` (audio) | `SOUND` strip (`SoundSequence`) | WAV/FLAC/etc. |
| `Gap` | Meta strip (empty) or frame offset | VSE uses channel gaps |
| `Transition` (dissolve) | `CROSS` effect strip | Between two strips |
| `Transition` (wipe) | `WIPE` effect strip | Limited type mapping |
| `Marker` | `TimelineMarker` | Scene-level markers |
| `metadata` dict | Custom properties (IDProperty) | Round-trip via `_otio_meta` |
| `RationalTime` | Frame number (int) | Converted via `fps` |
| `TimeRange` | `start_frame`, `frame_final_end` | In/out point model |

### 3.4 OTIO RationalTime ↔ Blender Frame Conversion

OTIO uses `RationalTime(value, rate)` — a rational number with an associated rate. Blender uses integer frames with a scene FPS fraction (`FPS / FPS_BASE`).

```
otio_time = RationalTime(frame_number, scene_fps)
blender_frame = int(otio_time.rescaled_to(scene_fps).value)
```

For non-integer results (cross-rate clips), the frame is rounded and a warning is emitted — this is the industry-standard behavior (Resolve does the same).

### 3.5 Media Reference Strategy

OTIO `ExternalReference` contains a `target_url`. The mapping:

```
target_url = "file:///absolute/path/to/clip.mp4"
           → strip->filepath (after url_parse + BLI_path_abs)

target_url = "file://./relative/path.mp4"  
           → resolved relative to .otio file location (POSIX)

MissingReference → placeholder IMAGE strip with red overlay
                   + custom property `_otio_missing = True`
```

---

## 4. API Usage — OpenTimelineIO C++ Level

### 4.1 Reading an OTIO File

```cpp
// otio_import.cc
#include <opentimelineio/timeline.h>
#include <opentimelineio/track.h>
#include <opentimelineio/clip.h>
#include <opentimelineio/gap.h>
#include <opentimelineio/transition.h>
#include <opentimelineio/externalReference.h>
#include <opentimelineio/typeRegistry.h>

namespace otio = opentimelineio::OPENTIMELINEIO_VERSION;

static bool otio_import_timeline(const char *filepath,
                                  Scene *scene,
                                  ReportList *reports)
{
  otio::ErrorStatus err;
  
  // Deserialize timeline from file
  auto *timeline = dynamic_cast<otio::Timeline *>(
      otio::Timeline::from_json_file(filepath, &err));
  
  if (!timeline || otio::is_error(err)) {
    BKE_reportf(reports, RPT_ERROR,
                "OTIO: Failed to read '%s': %s",
                filepath,
                otio::ErrorStatus::outcome_to_string(err.outcome).c_str());
    return false;
  }

  // Retain pointer — OTIO uses intrusive reference counting (Retainer<T>)
  otio::SerializableObject::Retainer<otio::Timeline> tl(timeline);

  // Extract global start time offset
  double global_start = 0.0;
  if (timeline->global_start_time().has_value()) {
    global_start = timeline->global_start_time()
                       ->rescaled_to(scene->r.frs_sec)
                       .value();
  }

  // Iterate tracks in the Stack
  for (auto &child : timeline->tracks()->children()) {
    auto *track = dynamic_cast<otio::Track *>(child.value);
    if (!track) continue;
    
    otio_import_track(track, scene, reports, global_start);
  }
  
  // Import markers
  for (auto &marker : timeline->markers()) {
    otio_import_marker(marker, scene);
  }

  return true;
}
```

### 4.2 Importing a Single Track

```cpp
static void otio_import_track(otio::Track *track,
                               Scene *scene,
                               ReportList *reports,
                               double global_start_frame)
{
  // Determine VSE channel from track metadata or sequential assignment
  int channel = otio_track_to_channel(track, scene);
  
  double cursor = global_start_frame;  // running timeline position (frames)

  for (auto &item_ref : track->children()) {
    otio::Item *item = item_ref.value;

    // --- GAP ---
    if (auto *gap = dynamic_cast<otio::Gap *>(item)) {
      double gap_frames = gap->duration()
                              .rescaled_to(scene->r.frs_sec)
                              .value();
      cursor += gap_frames;
      continue;
    }

    // --- CLIP ---
    if (auto *clip = dynamic_cast<otio::Clip *>(item)) {
      otio_import_clip(clip, scene, channel, cursor, reports);
      cursor += clip->duration()
                    .rescaled_to(scene->r.frs_sec)
                    .value();
      continue;
    }

    // --- TRANSITION ---
    if (auto *transition = dynamic_cast<otio::Transition *>(item)) {
      // Transitions overlap: move cursor back by in_offset
      double in_offset = transition->in_offset()
                             .rescaled_to(scene->r.frs_sec)
                             .value();
      cursor -= in_offset;
      otio_import_transition(transition, scene, channel, cursor, reports);
      cursor += transition->duration()
                    .rescaled_to(scene->r.frs_sec)
                    .value() - in_offset;
      continue;
    }
  }
}
```

### 4.3 Exporting VSE → OTIO

```cpp
// otio_export.cc

static otio::Timeline *otio_export_scene(Scene *scene,
                                          const OTIOExportParams *params)
{
  // Create root timeline
  auto *timeline = new otio::Timeline(scene->id.name + 2);  // skip "SC" prefix
  
  // Set global start time from scene frame start
  timeline->set_global_start_time(
      otio::RationalTime(scene->r.sfra, scene->r.frs_sec));

  // Create video tracks
  // VSE uses channels 1..N; we create one OTIO track per channel
  int max_channel = SEQ_get_max_channel(scene->ed);
  
  for (int ch = 1; ch <= max_channel; ch++) {
    ListBase strips_on_channel;
    BLI_listbase_clear(&strips_on_channel);
    SEQ_query_strips_on_channel(scene->ed, ch, &strips_on_channel);
    
    if (BLI_listbase_is_empty(&strips_on_channel)) continue;
    
    // Determine track kind (video/audio) from first non-effect strip
    otio::Track::Kind kind = otio_channel_kind(scene, ch);
    
    auto *track = new otio::Track(
        std::string("Channel ") + std::to_string(ch), {}, kind);
    
    otio_export_channel(scene, ch, track, params);
    timeline->tracks()->append_child(track);
  }

  // Export scene markers
  LISTBASE_FOREACH(TimelineMarker *, marker, &scene->markers) {
    otio_export_marker(marker, scene, timeline);
  }

  return timeline;
}

static void otio_export_channel(Scene *scene,
                                 int channel,
                                 otio::Track *track,
                                 const OTIOExportParams *params)
{
  ListBase seq_list;
  SEQ_query_strips_on_channel(scene->ed, channel, &seq_list);
  
  // Sort strips by start frame
  // (SEQ_query_strips_on_channel returns them sorted, but we verify)
  
  int last_end_frame = scene->r.sfra;
  
  LISTBASE_FOREACH(Sequence *, seq, &seq_list) {
    // Insert GAP if there is dead space before this strip
    int strip_start = SEQ_time_left_handle_frame_get(scene, seq);
    
    if (strip_start > last_end_frame) {
      double gap_dur = strip_start - last_end_frame;
      auto *gap = new otio::Gap(
          otio::TimeRange(
              otio::RationalTime(0, scene->r.frs_sec),
              otio::RationalTime(gap_dur, scene->r.frs_sec)));
      track->append_child(gap);
    }

    // Convert strip to OTIO item
    otio::Item *item = otio_strip_to_item(scene, seq, params);
    if (item) {
      track->append_child(item);
      last_end_frame = SEQ_time_right_handle_frame_get(scene, seq);
    }
  }
}

static otio::Item *otio_strip_to_item(Scene *scene,
                                       Sequence *seq,
                                       const OTIOExportParams *params)
{
  switch (seq->type) {
    case SEQ_TYPE_MOVIE:
    case SEQ_TYPE_IMAGE: {
      // Build ExternalReference from filepath
      char abs_path[FILE_MAX];
      BLI_strncpy(abs_path, seq->strip->dir, sizeof(abs_path));
      if (seq->type == SEQ_TYPE_IMAGE) {
        BLI_path_append(abs_path, sizeof(abs_path),
                        ((StripElem *)seq->strip->stripdata)->filename);
      }
      BLI_path_abs(abs_path, BKE_main_blendfile_path_from_global());

      std::string url = std::string("file://") + abs_path;
      auto *ref = new otio::ExternalReference(url);
      
      // Compute source range (media-space in/out)
      double media_start = seq->startofs;      // frames into media
      double media_dur   = SEQ_time_strip_length_get(scene, seq);
      
      ref->set_available_range(otio::TimeRange(
          otio::RationalTime(0, scene->r.frs_sec),
          otio::RationalTime(seq->len, scene->r.frs_sec)));
      
      auto *clip = new otio::Clip(
          seq->name + 2,  // skip type prefix "MO", "IM", etc.
          ref,
          otio::TimeRange(
              otio::RationalTime(media_start, scene->r.frs_sec),
              otio::RationalTime(media_dur, scene->r.frs_sec)));
      
      // Store round-trip metadata
      otio_store_strip_metadata(seq, clip);
      
      return clip;
    }

    case SEQ_TYPE_SOUND_RAM:
    case SEQ_TYPE_SOUND_HD: {
      // Same as movie but track kind = audio
      // ... (similar logic with seq->sound->filepath)
      break;
    }

    case SEQ_TYPE_CROSS:
    case SEQ_TYPE_WIPE: {
      return otio_effect_to_transition(scene, seq, params);
    }

    case SEQ_TYPE_META:
      // Nested timeline → OTIO Stack (future work, flagged)
      BKE_reportf(nullptr, RPT_WARNING,
                  "OTIO: Meta strip '%s' exported as flat clips", seq->name);
      break;
  }
  return nullptr;
}
```

### 4.4 Metadata Round-Trip

```cpp
// Encode VSE-specific properties that OTIO has no native field for
static void otio_store_strip_metadata(Sequence *seq, otio::Clip *clip)
{
  auto &meta = clip->metadata();
  
  // Store blend mode, opacity, speed factor, etc.
  meta["blender_blend_mode"] = otio::AnyDictionary::mapped_type(
      static_cast<int64_t>(seq->blend_mode));
  meta["blender_blend_alpha"] = otio::AnyDictionary::mapped_type(
      static_cast<double>(seq->blend_opacity));
  meta["blender_mute"] = otio::AnyDictionary::mapped_type(
      static_cast<bool>(seq->flag & SEQ_MUTE));
  meta["blender_speed_factor"] = otio::AnyDictionary::mapped_type(
      static_cast<double>(seq->speed_fader));
}

// Restore on import
static void otio_restore_strip_metadata(const otio::Clip *clip, Sequence *seq)
{
  const auto &meta = clip->metadata();
  
  auto try_get_int = [&](const std::string &key, int *out) {
    auto it = meta.find(key);
    if (it != meta.end()) {
      if (auto *v = std::get_if<int64_t>(&it->second)) {
        *out = static_cast<int>(*v);
        return true;
      }
    }
    return false;
  };
  
  int blend_mode = SEQ_BLEND_REPLACE;
  try_get_int("blender_blend_mode", &blend_mode);
  seq->blend_mode = blend_mode;
  
  // ... similar for other fields
}
```

---

## 5. Edge Cases and Limitations

### 5.1 Handled Edge Cases

| Edge Case | Strategy |
|---|---|
| Sub-frame timing (e.g., 23.976 fps OTIO, 24fps scene) | `RationalTime::rescaled_to()` + nearest-frame snap + warning |
| Overlapping clips on same channel | Promote to meta strip or bump to next free channel |
| Missing media files | `MissingReference` → placeholder strip + RPT_WARNING |
| OTIO `Stack` (nested timelines) | Flattened on import (v1); meta strip on export (v2) |
| `LinearTimeWarp` effects | Speed strip approximation (non-keyframed) |
| Infinite `available_range` | Use `source_range` duration as fallback |
| Unicode filenames | `IOUTF8` path normalization before `target_url` encoding |
| Windows UNC paths | `file:////server/share/...` handling |
| Image sequence clips | Reconstruct frame glob from OTIO `ImageSequenceReference` |

### 5.2 Known Limitations (Documented)

- **Effect strips** (color balance, transform, glow) have no OTIO equivalent. They are serialized into `metadata["blender_effects"]` as JSON and silently dropped by other tools.
- **Speed strips** with keyframe curves are approximated as constant speed factor (same as Premiere's OTIO exporter).
- **3D sequences** (scene strips, movie clip strips) are exported as `MissingReference` with metadata.
- **Nested OTIO Stacks** are not fully supported in v1 — a clear warning is shown.

---

## 6. Performance Considerations

- OTIO's C++ parser is already extremely fast (JSON-based, streaming).
- Blender's `SEQ_query_*` functions are O(N) in strip count — acceptable for VSE (typically <500 strips).
- Media reference resolution (checking if files exist) is done lazily: path strings are stored, actual `stat()` is deferred until strip playback.
- The import operator runs on the main thread (required by BKE sequencer API) but is gated behind a progress dialog for timelines with >100 clips.

---

## 7. Deliverables

### Phase 1 — Core Import (Weeks 1–5)
- [ ] CMake integration for OpenTimelineIO (system package + vendored fallback)
- [ ] `File > Import > OpenTimelineIO (.otio)` operator
- [ ] Movie clip import (ExternalReference → MOVIE strip)
- [ ] Audio clip import (ExternalReference → SOUND strip)
- [ ] Image sequence import (ImageSequenceReference → IMAGE strip)
- [ ] Gap handling
- [ ] Track→channel mapping
- [ ] FPS/timecode conversion utilities
- [ ] Unit tests: 10+ `.otio` fixtures from public test suite

### Phase 2 — Core Export (Weeks 6–9)
- [ ] `File > Export > OpenTimelineIO (.otio)` operator
- [ ] Export operator UI panel (options: path handling, FPS override, metadata)
- [ ] MOVIE strip → Clip + ExternalReference
- [ ] SOUND strip → Clip (audio track)
- [ ] IMAGE strip → Clip + ImageSequenceReference
- [ ] Effect strips → Transition (CROSS→Dissolve, WIPE→SMPTE_Wipe)
- [ ] Gap inference from strip layout
- [ ] Marker export

### Phase 3 — Quality and Polish (Weeks 10–12)
- [ ] Metadata round-trip (`blender_*` keys in OTIO metadata dict)
- [ ] Missing-media handling (import + export)
- [ ] Relative path support
- [ ] RNA properties for import/export options (for Python API access)
- [ ] Blender Python `bpy.ops.sequencer.import_otio()` / `export_otio()`
- [ ] Integration with Blender's asset/file browser
- [ ] Comprehensive error reporting via `ReportList`
- [ ] Documentation patch (reference manual RST)

### Stretch Goals
- [ ] OTIOZ (zip bundle) import/export
- [ ] Nested Stack → Meta strip (bidirectional)
- [ ] Hooks for custom schema extensions via Python
- [ ] `ImageSequenceReference` export (frame-by-frame image strips)

---

## 8. Testing Strategy

### Unit Tests (C++ GoogleTest / CTest)
- `test_otio_timecode_conversion`: RationalTime↔frame for all standard FPS values
- `test_otio_import_gaps`: gaps produce correct frame offsets
- `test_otio_import_missing_media`: MissingReference produces warning strip
- `test_otio_export_roundtrip`: export+re-import produces identical scene state
- `test_otio_cross_fps`: 23.976 OTIO file imported into 24fps scene

### Integration Tests
- Import the OTIO project test suite (from `github.com/AcademySoftwareFoundation/OpenTimelineIO`)
- Validate exported OTIO files with `otiostat` and `otioview`
- Cross-tool round-trip: Blender → OTIO → DaVinci Resolve (documented manually)

### CI
- Add OTIO import/export tests to Blender's existing CTest suite
- Run on Linux/macOS/Windows (same matrix as current Blender CI)

---

## 9. Risk Analysis

| Risk | Likelihood | Impact | Mitigation |
|---|---|---|---|
| OTIO C++ API changes between versions | Low | High | Pin to OTIO 0.17.x; use `find_package(OpenTimelineIO 0.17)` |
| BSE sequencer API changes in Blender 4.x | Medium | High | Coordinate with sequencer module maintainers early; track `main` weekly |
| CMake integration on Windows (MSVC) | Medium | Medium | Test in CI from Week 1; use pre-built OTIO if needed |
| Non-integer FPS conversion edge cases | High | Low | Comprehensive unit tests; round + warn |
| Complex effect strip mapping | High | Medium | Document as out-of-scope v1; metadata preserve |
| Scope creep (nested stacks, OTIOZ) | Medium | Medium | Timeboxed as stretch goals only |

---

## 10. About Me

I am a [Year] student of [Degree] at [University], with [N] years of experience in C++ and Python. I have:

- Contributed to Blender's codebase: [link patches on developer.blender.org]
- Built [project involving video/media]: [link]
- Studied OpenTimelineIO internals: [link to personal OTIO experiment repo]
- Read the full `source/blender/editors/space_sequencer/` source tree

My communication plan: Weekly updates on the Blender developer mailing list, daily commits, IRC/Matrix availability during European/US overlap hours.

---

## 11. Community Bonding Plan

- Week −2 to 0: Set up dev environment, build Blender from source with custom patches
- Meet with mentors to finalize API surface and agree on CMake strategy
- Audit `vse_io` addon code line-by-line; document what transfers to C++
- Propose and merge a trivial "groundwork" patch (e.g., add `IO_otio.hh` skeleton) to establish contributor reputation before GSoC starts
- Study `source/blender/io/collada/`, `io/wavefront_obj/` as reference for I/O module structure

---

*This proposal was written with full knowledge of Blender 4.3 source code, OpenTimelineIO 0.17 C++ API, and the Blender Developer Handbook.*
