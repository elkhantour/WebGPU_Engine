#include "./core.h"

#include <cglm/mat4.h>
#include <cglm/quat.h>
#include <cglm/vec3.h>
#include <stdlib.h>
#include <string.h>
#include <webgpu/webgpu.h>

#include "backend/logger.h"
#include "backend/registry.h"
#include "backend/std_pipeline/core.h"
#include "backend/ubo.h"
#include "runtime/pipeline/render.h"
#include "shader/core.h"
#include "topology/boundbox.h"
#include "uniform.h"
#include "utils/dyli.h"
#include "utils/name.h"

// Shadow map is implicitely handled withing mesh
static inline Mesh *mesh_children_list_check_init(Mesh *);

void mesh_create(Mesh *mesh, const MeshCreateDescriptor *md) {

  mesh->id = reg_register(mesh, RegEntryType_Mesh);

  // set name
  mesh_set_name(mesh, md->name);

#ifdef VERBOSE_CREATING_PHASE
  logger_add(LoggerFlag_MeshCreate, "%s", mesh->name);
#endif

  // init child list
  mesh->children.count = 0;
  mesh->children.capacity = MESH_CHILD_COUNT;
  mesh->children.entries = NULL;

  // set vertices & index for base topology
  if (md->vertex.count > 0 && md->index.count > 0) {
    mesh_topology_base_create(&mesh->topology.base, &md->vertex, &md->index);
    mesh_topology_base_upload(&mesh->topology.base);
  }

  // init model matrix and transforms
  glm_mat4_identity(mesh->model);

  glm_quat_identity(mesh->rotation_quat);

  glm_vec3_copy(GLM_VEC3_ZERO, mesh->position);
  glm_vec3_copy(GLM_VEC3_ZERO, mesh->rotation_euler);
  glm_vec3_copy(GLM_VEC3_ONE, mesh->scale);

  // defines default topology override
  mesh_topology_set_override(mesh,
                             (MeshTopology){
                                 .attribute = &mesh->topology.base.attribute,
                                 .index = &mesh->topology.base.index,
                             });
}

/**
   Create mesh from primitive index and vertex attributes
 */
void mesh_create_primitive(Mesh *mesh,
                           const MeshCreatePrimitiveDescriptor *md) {

  mesh_create(mesh, &(MeshCreateDescriptor){
                        .index = md->primitive->index,
                        .vertex = md->primitive->vertex,
                        .name = md->name,
                    });
}

void mesh_set_parent(Mesh *child, Mesh *parent) { child->parent = parent; }

/**
   Check if children list is already created.
   If not init a new list
 */
Mesh *mesh_children_list_check_init(Mesh *parent) {

  if (parent->children.entries == NULL)
    dyli_create((void *)&parent->children.entries, &parent->children.capacity,
                &parent->children.count, sizeof(Mesh *), 16,
                "Mesh children list");

  return *parent->children.entries;
}

Mesh *mesh_child_new(Mesh *parent) {

  // init list
  mesh_children_list_check_init(parent);

  Mesh *child = dyli_new_entry(
      (void *)&parent->children.entries, &parent->children.capacity,
      &parent->children.count, sizeof(Mesh *), "Mesh child list");

  if (child == NULL) {
    logger_add(LoggerFlag_Warning,
               "Couldn't create new entry in mesh child list.");
    return NULL;
  }

  child->parent = parent;

  return child;
}

/**
Add a new child pointer to the destination mesh children list
 */
DynamicListStatus mesh_child_add(Mesh *parent, Mesh *child) {

  mesh_children_list_check_init(parent);

  DynamicListStatus insert =
      dyli_insert((void *)&parent->children.entries, &parent->children.capacity,
                  &parent->children.count, sizeof(Mesh *), (void *)&child, 1,
                  "Mesh children list");

  if (insert == DynamicListStatus_Success) {
    // assign parent pointer to child
    child->parent = parent;
  }

  return insert;
}

void mesh_destroy(Mesh *mesh) {
  // TODO
}
