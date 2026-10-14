#ifndef _MESH_DRAW_H_
#define _MESH_DRAW_H_

// Compact mesh data used in draw call to improve cache locality and prevent
// cache miss
#include "backend/registry.h"
#include "core.h"
#include "runtime/pipeline/render.h"
#include "runtime/shader/core.h"
#include "runtime/shader/draw.h"
#include "utils/defines.h"
#include "utils/dyli.h"
#include "utils/name.h"
#include "webgpu/webgpu.h"

typedef struct {
  // --- used in hot loop / compulsory used ---
  size_t count;
  WGPUBuffer attribute;
  WGPUBuffer index;
  ShaderBindGroupList *bindgroup_list;
  // --- cold path / conditionnaly used ---
  Mesh *mesh;              // needed for preprocessor callback
  const char *shader_name; // needed for debug purpose
} MeshDrawPacket;

typedef struct {
  MeshDrawPacket *entries;
  size_t count;
  size_t capacity;
} MeshDrawPacketList;

EXTERN_C_BEGIN

static inline DynamicListStatus
mesh_draw_packet_list_create(MeshDrawPacketList *list, size_t capacity) {
  return dyli_create((void **)&list->entries, &list->capacity, &list->count,
                     sizeof(MeshDrawPacket), capacity, "Mesh Draw Packet List");
}

static inline MeshDrawPacket *
mesh_draw_packet_list_new_entry(MeshDrawPacketList *list) {
  MeshDrawPacket *entry = (MeshDrawPacket *)dyli_new_entry(
      (void **)&list->entries, &list->capacity, &list->count,
      sizeof(MeshDrawPacket), "Mesh Draw Packet List");

  return entry;
}

static inline DynamicListStatus
mesh_draw_packet_list_remove(MeshDrawPacketList *list, MeshDrawPacket *entry) {
  return dyli_remove((void *)list->entries, &list->count,
                     sizeof(MeshDrawPacket), (void *)entry,
                     "Mesh Draw Packet List");
}

static inline DynamicListStatus
mesh_draw_packet_list_destroy(MeshDrawPacketList *list) {
  return dyli_free((void **)&list->entries, &list->capacity, &list->count);
}

static inline DynamicListStatus
mesh_draw_packet_list_remove_at_index(MeshDrawPacketList *list, size_t index) {
  return dyli_remove_at_index((void *)list->entries, &list->count,
                              sizeof(MeshDrawPacket), index,
                              "Mesh Draw Packet List");
}

static inline DynamicListStatus
mesh_draw_packet_list_empty(MeshDrawPacketList *list) {
  return dyli_empty((void *)list->entries, &list->count,
                    sizeof(MeshDrawPacket));
}

static inline MeshDrawPacket *
mesh_draw_packet_list_find_mesh(MeshDrawPacketList *list, Mesh *mesh,
                                   size_t *index) {

  for (size_t i = 0; i < list->count; i++)
    if (list->entries[i].mesh->id == mesh->id) {
      if (index)
        *index = i;
      return &list->entries[i];
    }

  if (index)
    *index = DYLI_INVALID_INDEX;

  return NULL;
}

static inline void mesh_create_draw_packet(MeshTopology topo, Shader *shader,
                                           Mesh *mesh, MeshDrawPacket *pack) {
  pack->attribute = topo.attribute->buffer;
  pack->index = topo.index->buffer;
  pack->count = topo.index->count;
  pack->bindgroup_list = &shader->bind_groups;
  pack->shader_name = shader->name;
  pack->mesh = mesh;
}

/**
   Mesh main draw from default vertex and index buffer
 */
static inline void mesh_draw(MeshDrawPacket *pack,
                             WGPURenderPassEncoder render_pass) {

  WGPUBuffer attribute_buffer = pack->attribute;
  WGPUBuffer index_buffer = pack->index;
  size_t index_count = pack->count;

  // draw indexes from buffer
  wgpuRenderPassEncoderSetVertexBuffer(render_pass, 0, attribute_buffer, 0,
                                       WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetIndexBuffer(render_pass, index_buffer,
                                      MESH_INDEX_FORMAT, 0, WGPU_WHOLE_SIZE);
  wgpuRenderPassEncoderSetStencilReference(render_pass, 1);
  wgpuRenderPassEncoderDrawIndexed(render_pass, index_count, 1, 0, 0, 0);
}

EXTERN_C_END

#endif
