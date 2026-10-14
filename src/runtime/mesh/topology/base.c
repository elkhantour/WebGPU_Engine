#include "base.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "anchor.h"
#include "backend/logger.h"
#include "backend/resource_manager.h"
#include "runtime/geometry/vertex/core.h"
#include "runtime/geometry/vertex/transform.h"
#include "webgpu/webgpu.h"

static void mesh_topology_base_create_anchor(MeshTopologyBase *);

/**
   Handle the base topology creation as well as anchor generation
 */
MeshTopologyBaseStatus mesh_topology_base_create(MeshTopologyBase *base,
                                                 const VertexAttribute *va,
                                                 const VertexIndex *vi) {

  // create vertex attributes
  mesh_topology_base_create_vertex_attribute(base, va);

  // create index attributes
  mesh_topology_base_create_vertex_index(base, vi);

  // create anchors
  mesh_topology_base_create_anchor(base);

  return MeshTopologyBaseStatus_Success;
}

MeshTopologyBaseStatus mesh_topology_base_upload(MeshTopologyBase *base) {

  if (!base->attribute.count || !base->index.count) {
    logger_add(LoggerFlag_Error,
               "Attempting to upload empty vertices buffers.");
    return MeshTopologyBaseStatus_Empty;
  }

  {
    // vert attribute upload
    if (base->attribute.buffer != NULL)
      rem_destroy_buffer(&base->attribute.buffer);

    const size_t va_size = base->attribute.count * sizeof(vattr_t);

    base->attribute.buffer = rem_new_buffer(&(WGPUBufferDescriptor){
        .label = "Base Topology Vertex Attributes",
        .mappedAtCreation = false,
        .size = va_size,
        .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
    });

    rem_write_buffer(base->attribute.buffer, 0, (void *)base->attribute.entries,
                     va_size, REMWriteFlag_None);
  }

  {
    // vert index upload
    if (base->index.buffer != NULL)
      rem_destroy_buffer(&base->index.buffer);

    const size_t vi_size = base->index.count * sizeof(vindex_t);

    base->index.buffer = rem_new_buffer(&(WGPUBufferDescriptor){
        .label = "BoundBox Topology Vertex Indexes",
        .mappedAtCreation = false,
        .size = vi_size,
        .usage = WGPUBufferUsage_Index | WGPUBufferUsage_CopyDst,
    });

    rem_write_buffer(base->index.buffer, 0, (void *)base->index.entries,
                     vi_size, REMWriteFlag_None);
  }
  return MeshTopologyBaseStatus_Success;
}

/**
   Return the MeshTopology of base vertex (without anchor)
 */
MeshTopology mesh_topology_base_vertex(MeshTopologyBase *topo) {
  return (MeshTopology){
      .attribute = &topo->attribute,
      .index = &topo->index,
  };
}

/**
   Create the base vertex attributes and upload data to buffer
 */
MeshTopologyBaseStatus
mesh_topology_base_create_vertex_attribute(MeshTopologyBase *base,
                                           const VertexAttribute *va) {

  base->attribute.count = va->count;
  base->attribute.capacity = va->capacity;

  // copy vertex attributes
  size_t vattr_size = va->capacity * sizeof(vattr_t);
  base->attribute.entries = calloc(va->capacity, sizeof(vattr_t));
  memcpy(base->attribute.entries, va->entries, vattr_size);

  return MeshTopologyBaseStatus_Empty;
}

/**
   Create the base index attributes and upload data to buffer
 */
MeshTopologyBaseStatus
mesh_topology_base_create_vertex_index(MeshTopologyBase *base,
                                       const VertexIndex *vi) {

  base->index.count = vi->count;
  base->index.capacity = vi->capacity;

  const size_t vi_capacity = vi->capacity * sizeof(vindex_t);
  base->index.entries = calloc(vi->capacity, sizeof(vindex_t));
  memcpy(base->index.entries, vi->entries, vi_capacity);

  return MeshTopologyBaseStatus_Empty;
}

/**
   Cache siblings anchor for each vertex.
 */
void mesh_topology_base_create_anchor(MeshTopologyBase *base) {

  MeshTopologyAnchorList hashed_list; // temp

  // init new list
  mesh_topology_anchor_list_create(&hashed_list,
                                   MESH_TOPOLOGY_ANCHOR_LIST_DEFAULT_CAPACITY);

  // 1. store based on position (hash)
  for (size_t i = 0; i < base->index.count; i++) {
    vindex_t base_index = base->index.entries[i];
    vattr_t *base_vertex = &base->attribute.entries[base_index * VERTEX_STRIDE];

    vec3 position;
    memcpy(&position, base_vertex, sizeof(vertex_position));

    mesh_topology_anchor_list_insert(&hashed_list, &position, &base_index, 1);
  }

  // 2. remmap based on index (linear)
  MeshTopologyAnchorList *mapped_list = &base->siblings;

  // destroy if already exists
  if (mapped_list->entries != NULL)
    mesh_topology_anchor_list_destroy(mapped_list);

  // create mapped list
  mesh_topology_anchor_list_create(mapped_list,
                                   MESH_TOPOLOGY_ANCHOR_LIST_DEFAULT_CAPACITY);
  // remap
  MeshTopology base_topo = mesh_topology_base_vertex(base);
  mesh_topology_anchor_list_map(&hashed_list, &base_topo, mapped_list);
}

void mesh_topology_base_set_scale(MeshTopologyBase *base,
                                  const VertexGroup *select, vec3 *scale) {

  // combine all anchors
  MeshTopologyAnchor combined_anchor;
  mesh_topology_anchor_create(&combined_anchor,
                              MESH_TOPOLOGY_ANCHOR_DEFAULT_CAPACITY);

  MeshTopologyAnchorList *anchors = &base->siblings;

  for (size_t i = 0; i < select->count; i++) {
    vindex_t index = select->entries[i];
    MeshTopologyAnchor *index_anchor = &anchors->entries[index];
    mesh_topology_anchor_merge(anchors, index_anchor->entries,
                               index_anchor->count, &combined_anchor);
  }

  // apply transform with all combined anchors
  vertex_transform_set_scale(
      &(VertexGroup){
          .entries = combined_anchor.entries,
          .count = combined_anchor.count,
      },
      &base->attribute, scale);
}

void mesh_topology_base_set_position(MeshTopologyBase *base,
                                     const VertexGroup *select,
                                     vec3 *translate) {

  // combine all anchors
  MeshTopologyAnchor combined_anchor;
  mesh_topology_anchor_create(&combined_anchor,
                              MESH_TOPOLOGY_ANCHOR_DEFAULT_CAPACITY);

  MeshTopologyAnchorList *anchors = &base->siblings;

  for (size_t i = 0; i < select->count; i++) {
    vindex_t index = select->entries[i];
    MeshTopologyAnchor *index_anchor = &anchors->entries[index];
    mesh_topology_anchor_merge(anchors, index_anchor->entries,
                               index_anchor->count, &combined_anchor);
  }

  // apply transform with all combined anchors
  vertex_transform_set_position(
      &(VertexGroup){
          .entries = combined_anchor.entries,
          .count = combined_anchor.count,
      },
      &base->attribute, translate);
}

/*
  Refresh attribute and index buffer.
  Used if vertex and index changes and need to update the buffer to reflect new
  data.
 */
void mesh_topology_base_update_buffer(MeshTopologyBase *base) {

  {
    // === Vertex attributes ===
    const size_t va_size = base->attribute.count * sizeof(vattr_t);
    rem_write_buffer(base->attribute.buffer, 0, (void *)base->attribute.entries,
                     va_size, REMWriteFlag_None);
  }

  {
    // === Vertex indexes ===
    const size_t vi_size = base->index.count * sizeof(vindex_t);
    rem_write_buffer(base->index.buffer, 0, (void *)base->index.entries,
                     vi_size, REMWriteFlag_None);
  }
}
