# VSE IO Addon Audit (2021 Python)

The 2021 vse_io addon from Blender Studio:

Pros:
- Proof of concept for OTIO → VSE strips

Cons:
| Feature | Status |
|---------|--------|
| Audio | Missing |
| Transitions | Not implemented |
| Speed | Python slow for large timelines |

Lessons: Use BKE_sequence_add_* for strips, but need C++ for performance/file menu.

COMPLETE analysis.
