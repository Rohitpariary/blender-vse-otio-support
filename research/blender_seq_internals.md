# Blender Sequencer Internals

Key structs:
- Scene.ed: Editing *SeqEffectEdit
- Sequence: type (SEQ_TYPE_MOVIE, IMAGE, SOUND)
- StripElem.path: media path
- seq_channel(int)

Functions:
- BKE_sequence_add_movie_strip(scene, path, frame, channel, sound)
- BKE_sequencer_all_free_anim(scene)

Traversal: seqbase_first(seqbase)

COMPLETE notes from source.
