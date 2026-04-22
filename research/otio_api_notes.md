# OTIO C++ API Notes

- Use opentimelineio::Timeline::from_json_file(path)
- RationalTime.value / rate for frame calc (drop frame aware?)
- Serializing: obj->to_json_file(path, indent=2)
- Gotchas: Namespaces, exceptions on invalid JSON.

Examples:
auto tl = Timeline::from_json_file(\"test.otio\");
for (auto child : tl->tracks()) { ... }

COMPLETE notes.
