#include "scale.h"

#include "./utils.h"
#include "core.h"
#include "runtime/mesh/core.h"
#include "utils/defines.h"

void gizmo_scale_create(MeshRefList *visual_list, MeshRefList *interactive_list,
                        const GizmoCreateDescriptor *desc) {

  gizmo_create_handles(visual_list, interactive_list,
                       &(GizmoCreateMeshDescriptor){
                           .mbin_path = RESOURCES_PATH_MBIN("scale.mbin"),
                           .offset = {0.0f, 0.1f, 0.0f},
                       });
}
