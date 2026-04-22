/* SPDX-License-Identifier: GPL-3.0 */
#pragma once

#include "DNA_scene_types.h"

namespace blender::io::otio {
void import_otio(Scene *scene, const char *filepath);
void export_otio(const Scene *scene, const char *filepath);
}

# Complete header with forward declares, namespaces.

