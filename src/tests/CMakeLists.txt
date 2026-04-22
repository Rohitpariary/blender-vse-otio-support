find_package(GTest REQUIRED)
add_executable(test_timecode test_timecode.cc)
target_link_libraries(test_timecode gtest bf_io_otio)
add_test(NAME test_timecode COMMAND test_timecode)

add_executable(test_import test_import.cc)
target_link_libraries(test_import gtest bf_io_otio)
add_test(NAME test_import COMMAND test_import)

add_executable(test_export test_export.cc)
target_link_libraries(test_export gtest bf_io_otio)
add_test(NAME test_export COMMAND test_export)

# COMPLETE CMake for tests
