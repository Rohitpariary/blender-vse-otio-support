# Technical Design Document
## VSE: OpenTimelineIO Support — Blender GSoC 2026

**Status:** Draft v0.3  
**Author:** [Your Name]  
**Reviewers:** [Mentor Names TBD]  
**Last Updated:** 2026-03-01

---

## 1. Blender VSE Internals

### 1.1 Data Model

The VSE's core data lives in `Editing` (defined in `DNA_sequence_types.h`):

```
Scene
└── ed (Editing *)
    ├── seqbase (ListBase)   ← all top-level Sequence strips
    ├── metastack (ListBase) ← stack for entering meta strips
    └── act_seq (Sequence *) ← currently selected strip

Sequence (a single strip)
├── type         (SEQ_TYPE_MOVIE, SEQ_TYPE_SOUND_RAM, SEQ_TYPE_IMAGE, ...)
├── machine      (int)  ← channel number, 1-indexed
├── start        (int)  ← timeline start frame (content start)
├── startofs     (int)  ← trim from media start (in-point offset)
├── endofs       (int)  ← trim from media end (out-point offset)
├── len          (int)  ← media duration in frames
├── flag         (int)  ← SEQ_MUTE, SEQ_LOCK, SEQ_SELECTED, etc.
├── blend_mode   (int)  ← SEQ_BLEND_REPLACE, SEQ_BLEND_ADD, etc.
├── blend_opacity(float)
├── name[64]     (char) ← prefixed: "MO", "IM", "SO", "SC", etc.
├── strip        (Strip *)
│   ├── dir[FILE_MAX]  ← directory of source media
│   └── stripdata      ← StripElem* for images, or movie handle
├── seq1, seq2, seq3   ← input strips for effect strips
└── seqbase (ListBase)  ← children (only for SEQ_TYPE_META)
```

### 1.2 Time Model

Blender VSE uses **integer frames** as the canonical time unit. Scene FPS is stored as:
```c
scene->r.frs_sec       // numerator (e.g., 24)
scene->r.frs_sec_base  // denominator (e.g., 1.0 for 24fps, 1001.0 for 23.976fps)
// Actual FPS = frs_sec / frs_sec_base
```

Strip timing:
```
Timeline frame of strip start = seq->start  (content-relative)
Media in-point              = seq->startofs (frames into media to skip)
Media out-point cutoff      = seq->len - seq->endofs
Displayed frame range       = [seq->start, seq->start + displayed_length - 1]
```

The Blender 4.x refactor moved some timing functions to:
- `SEQ_time_left_handle_frame_get(scene, seq)` → first displayed frame
- `SEQ_time_right_handle_frame_get(scene, seq)` → last displayed frame + 1
- `SEQ_time_strip_length_get(scene, seq)` → displayed duration in frames

### 1.3 BKE Sequencer API (Public)

Key functions used in this project:

```c
// Creation
Sequence *SEQ_sequence_alloc(ListBase *lb, int timeline_frame, int machine, int type);
void SEQ_add_movie_sequence(Main *bmain, Scene *scene, Sequence *seq, SeqLoadData *load_data);
void SEQ_add_sound_sequence(Main *bmain, Scene *scene, Sequence *seq, SeqLoadData *load_data);
void SEQ_add_image_sequence(Main *bmain, Scene *scene, Sequence *seq, SeqLoadData *load_data);

// Query
void SEQ_query_strips_on_channel(Editing *ed, int channel, ListBase *r_strips);
int  SEQ_get_max_channel(Editing *ed);
void SEQ_sort(ListBase *seqbase); // sort by start frame

// Timing (Blender 4.x)
int SEQ_time_left_handle_frame_get(const Scene *scene, const Sequence *seq);
int SEQ_time_right_handle_frame_get(const Scene *scene, const Sequence *seq);
int SEQ_time_strip_length_get(const Scene *scene, const Sequence *seq);

// Effect strips
Sequence *SEQ_effect_handle_get(Sequence *seq);
const SeqEffectHandle *SEQ_effect_get_sequence_blend(Sequence *seq);
```

### 1.4 How to Add a New File I/O Operator

Reference: `source/blender/io/wavefront_obj/` and `source/blender/io/collada/`

Pattern:
1. Define operator type `WM_OT_*` in `editors/space_sequencer/sequencer_io_otio.cc`
2. Register via `WM_operatortype_append()` in `sequencer_ops.cc`
3. Add to File menu in `sequencer_menus.cc`
4. Actual I/O logic lives in `io/otio/intern/` (separate module)

The split between `editors/` and `io/` ensures the core conversion code has no UI dependency and can be called from Python or other operators.

---

## 2. OpenTimelineIO Schema Reference

### 2.1 Object Hierarchy

```
SerializableObject
├── SerializableObjectWithMetadata  ← adds .metadata (AnyDictionary)
│   ├── Composable                  ← can live inside a composition
│   │   ├── Item                    ← has source_range, effects, markers
│   │   │   ├── Composition         ← contains children
│   │   │   │   ├── Stack           ← parallel tracks (root of Timeline)
│   │   │   │   └── Track           ← sequential items + kind (video/audio)
│   │   │   ├── Clip                ← has media_reference
│   │   │   └── Gap                 ← empty space
│   │   └── Transition              ← dissolve, wipe, etc.
│   ├── MediaReference              ← points to media
│   │   ├── ExternalReference       ← file:// URL
│   │   ├── ImageSequenceReference  ← frame glob
│   │   ├── GeneratorReference      ← procedural (solid color, etc.)
│   │   └── MissingReference        ← offline/unknown media
│   ├── Effect                      ← applied to Item
│   │   ├── TimeEffect
│   │   │   ├── LinearTimeWarp      ← speed factor
│   │   │   └── FreezeFrame
│   │   └── Effect                  ← base (custom)
│   └── Marker                      ← named point in time with color
└── Timeline                        ← top-level document object
    ├── name (str)
    ├── global_start_time (RationalTime, optional)
    └── tracks (Stack)              ← the root Stack
```

### 2.2 RationalTime

```cpp
// A time value expressed as: value / rate
otio::RationalTime t(48, 24);  // = frame 48 at 24fps = 2.0 seconds

// Rescale between rates
otio::RationalTime t_30fps = t.rescaled_to(30.0);  // → RationalTime(60, 30)

// Convert to seconds
double secs = t.to_seconds();  // → 2.0

// From seconds
otio::RationalTime from_secs = otio::RationalTime::from_seconds(2.0, 24.0);
```

### 2.3 TimeRange

```cpp
// start_time + duration (both RationalTime at same rate)
otio::TimeRange range(
    otio::RationalTime(10, 24),  // start: frame 10
    otio::RationalTime(50, 24)   // duration: 50 frames
);

// end_time_exclusive() = start + duration
// end_time_inclusive() = start + duration - 1 frame
range.end_time_exclusive();  // → RationalTime(60, 24) 

// Duration
range.duration();  // → RationalTime(50, 24)
```

### 2.4 Track Kinds

```cpp
otio::Track::Kind::Video  // "Video"
otio::Track::Kind::Audio  // "Audio"
// Default is Video if not specified
```

### 2.5 Transition Model

OTIO transitions are placed **between** two adjacent clips. They are children of a Track alongside the clips. The transition has:
- `in_offset`: how much it extends backward into the preceding clip
- `out_offset`: how much it extends forward into the following clip

```
Track children: [Clip A] [Transition] [Clip B]

Timeline: |--Clip A--|
                  |--Transition--|
                         |--Clip B--|

Clip A must extend past its apparent end by `in_offset`
Clip B must begin before its apparent start by `out_offset`
```

In Blender VSE, effect strips (CROSS, WIPE) sit on a third channel **above** the two input strips and cover the overlap region. The conversion requires careful handle detection.

---

## 3. Data Conversion Strategy

### 3.1 Import: OTIO → VSE

```
otio_import.cc

otio_import_timeline(filepath, scene)
  └── parse Timeline from JSON
  └── for each Track in Stack:
        └── otio_import_track(track, scene, channel_idx)
              └── cursor = global_start_frame
              └── for each item in track:
                    ├── Gap      → advance cursor
                    ├── Clip     → otio_import_clip(clip, scene, ch, cursor)
                    │              └── detect media type from reference
                    │              └── call appropriate BKE add function
                    │              └── set timing from source_range
                    │              └── otio_restore_strip_metadata()
                    │              → advance cursor by clip duration
                    └── Transition → otio_import_transition(...)
                                     └── find preceding + following strips
                                     └── create CROSS/WIPE effect strip
                                     → adjust cursor
```

### 3.2 Export: VSE → OTIO

```
otio_export.cc

otio_export_scene(scene, params)
  └── create Timeline(scene name)
  └── for each channel 1..max:
        └── get strips on channel (sorted by start frame)
        └── create Track(channel_N, kind)
        └── otio_export_channel(scene, ch, track, params)
              └── last_end = scene start frame
              └── for each strip on channel:
                    ├── if gap before strip: append Gap
                    ├── MOVIE/IMAGE/SOUND → otio_strip_to_clip()
                    │     └── build ExternalReference from filepath
                    │     └── compute source_range from startofs/len/endofs
                    │     └── otio_store_strip_metadata()
                    └── CROSS/WIPE → otio_effect_to_transition()
                          └── create Transition with in/out offsets
                          └── modify preceding/following clips in track
                             (extend their source ranges to cover overlap)
```

### 3.3 Channel → Track Mapping

Blender VSE channels are numbered 1..N with no inherent video/audio distinction. Detection heuristic:

```cpp
otio::Track::Kind otio_channel_kind(Scene *scene, int channel)
{
  // Walk strips on channel; first non-effect strip determines kind
  ListBase strips;
  SEQ_query_strips_on_channel(scene->ed, channel, &strips);
  
  LISTBASE_FOREACH(Sequence *, seq, &strips) {
    if (seq->type == SEQ_TYPE_SOUND_RAM) return otio::Track::Kind::Audio;
    if (seq->type == SEQ_TYPE_MOVIE)    return otio::Track::Kind::Video;
    if (seq->type == SEQ_TYPE_IMAGE)    return otio::Track::Kind::Video;
    // effect strips are skipped
  }
  return otio::Track::Kind::Video;  // default
}
```

### 3.4 Metadata Schema

All Blender-specific data is stored under the key `"blender"` in OTIO's `metadata` dictionary to avoid namespace collisions:

```json
{
  "OTIO_SCHEMA": "Clip.1",
  "metadata": {
    "blender": {
      "blend_mode": 0,
      "blend_opacity": 1.0,
      "mute": false,
      "speed_factor": 1.0,
      "use_proxy": false,
      "channel": 3
    }
  }
}
```

---

## 4. CMake Integration

### 4.1 System Package (preferred)

```cmake
# build_files/cmake/platform/platform_unix.cmake
if(WITH_IO_OTIO)
  find_package(OpenTimelineIO 0.17 REQUIRED
    HINTS ${OPENTIMELINEIO_ROOT_DIR}
  )
  if(NOT OpenTimelineIO_FOUND)
    message(FATAL_ERROR "OpenTimelineIO not found. "
            "Install it or set OPENTIMELINEIO_ROOT_DIR.")
  endif()
endif()
```

### 4.2 Vendored Fallback

```cmake
# extern/opentimelineio/CMakeLists.txt
if(NOT OpenTimelineIO_FOUND)
  message(STATUS "OTIO: using vendored copy")
  set(OTIO_DEPENDENCIES_INSTALL OFF)
  set(OTIO_FIND_IMATH OFF)
  add_subdirectory(src)  # git submodule
endif()
```

### 4.3 Module CMakeLists

```cmake
# source/blender/io/otio/CMakeLists.txt
if(WITH_IO_OTIO)
  set(INC
    .
    ../../blenkernel
    ../../makesdna
    ../../makesrna
    ../../windowmanager
    ../../../../intern/guardedalloc
  )
  
  set(SRC
    intern/otio_import.cc
    intern/otio_export.cc
    intern/otio_utils.cc
    IO_otio.hh
  )
  
  set(LIB
    bf_blenkernel
    bf_blenlib
    OpenTimelineIO::opentime
    OpenTimelineIO::opentimelineio
  )
  
  blender_add_lib(bf_io_otio "${SRC}" "${INC}" "" "${LIB}")
endif()
```

---

## 5. File Format Notes

### 5.1 .otio (JSON)

Standard OTIO on-disk format. Human-readable, diff-friendly. Always UTF-8.

```json
{
  "OTIO_SCHEMA": "Timeline.1",
  "name": "My Timeline",
  "global_start_time": {"OTIO_SCHEMA": "RationalTime.1", "rate": 24, "value": 0},
  "tracks": {
    "OTIO_SCHEMA": "Stack.1",
    "children": [
      {
        "OTIO_SCHEMA": "Track.1",
        "kind": "Video",
        "name": "Channel 1",
        "children": [...]
      }
    ]
  }
}
```

### 5.2 .otioz (Zip Bundle)

An OTIO file bundled with all referenced media inside a ZIP archive. Useful for project portability. The OTIO C++ API has a `ZipAdapter` for this. Planned as stretch goal.

### 5.3 Adapter Files (.py)

OTIO supports third-party adapters written in Python (e.g., `cmx_3600` for EDL, `fcp_xml` for FCP7). Blender's built-in integration uses the C++ API directly for performance, but could optionally invoke Python adapters if detected — noted for future extensibility.

---

## 6. Open Questions for Mentor Review

1. Should the import operator create a **new scene** or import into the active scene's sequencer?
   - Proposed: import into active scene, with option to create new scene.

2. Should we vendor OTIO or require it as a system dependency?
   - Proposed: system dependency with clear error message + link to install docs. Vendored in `extern/` only for official Blender builds.

3. For Blender's release builds: does OTIO need to be LGPL-compatible?
   - OTIO is Apache 2.0 → compatible with Blender GPL v3. ✓

4. Should the Python `bpy.ops.sequencer.import_otio()` call go through RNA or the C++ API directly?
   - Proposed: RNA (standard pattern for all Blender operators).

5. Naming convention for operator: `SEQUENCER_OT_import_otio` or `WM_OT_import_otio`?
   - Proposed: `SEQUENCER_OT_import_otio` (scoped to sequencer, consistent with `SEQUENCER_OT_export_sequencer`).
