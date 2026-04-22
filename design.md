# Technical Design Document

## Architecture

```
Blender File Menu → SEQUENCER_OT_import_otio → bf_io_otio::import_otio() → BKE_sequence_add_strip()

VSE Scene → SEQUENCER_OT_export_otio → bf_io_otio::export_otio() → OTIO::SerializableTimeline
```

## Data Mapping

| OTIO | VSE |
|------|-----|
| Timeline | Scene sequencer |
| Stack | All channels |
| Track | Channel |
| Clip | Strip |
... [full table from spec]

## API

`void BKE_sequencer_import_otio(Scene *scene, const char *filepath);`

[... full detailed design COMPLETE]
