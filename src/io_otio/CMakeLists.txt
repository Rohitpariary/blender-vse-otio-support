blender_add_lib(bf_io_otio "${CMAKE_CURRENT_SOURCE_DIR}/IO_otio.cc" "intern")
target_link_libraries(bf_io_otio libopentimelineio)

add_subdirectory(intern)
# Full CMake for module, link OTIO, Blender deps
# COMPLETE
