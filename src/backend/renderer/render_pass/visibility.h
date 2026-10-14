#ifndef _RENDER_PASS_VISBILITY_H_
#define _RENDER_PASS_VISBILITY_H_

#include "backend/logger.h"
#include "backend/std_pipeline/core.h"
#include "core.h"
#include "runtime/mesh/draw.h"
#include "runtime/mesh/shader/core.h"
#include "utils/dyli.h"

/* ===  Visbility manager ===

  The below functions try to provide an interface to help hidding or showing
  meshes from the render passes.

  Render Pass are composed of  "Layouts" lists.
  Layouts are basically meshes coupled with topology (wireframe, solid,
  boundbox) and shader (texture, solid, wireframe) callbacks.
  This trio is combined and used to be drawn together.

  Think of each layout as:

        "Draw this [mesh] with this [topology] and thi [shader]"

  Each layout is composed of two mesh lists to be drawn:

  - The Source: a constant list that is a shared reference amongst many layout.
    The source list usually comes from the scene pipelines (Lit meshes,
    unlit, shadowed) and can be seen as a fixed stagging list.

  - The Drawn: a more dynamic list in which we actually decide which meshes from
    the source list we want to draw from.

   This double layering ensure consistency accross multiple layouts by always
   referencing a same common source list while having the flexbility
   "per-layout" to render or not some meshes independtently.


   For information, priorly the engine used a "one layer" layout system, meaning
   each layout was directly linked and would show the source list meshes.
   However we quickly noticed that sometime we may want to hide the mesh from a
   layout but not from the whole shared source list.

   Sync the source mesh list with the actual draw list of the pass.
   We cannot draw directly the source mesh list (linked from the scene
   pipeline) because in some cases we need to hide of show some meshes in
   individual render pass.

   As instance, for probe reflection we may want to prevent self reflection
   and need to remove somes meshes from the pass. However if we remove those
   meshes from the source mesh list, then it means we also remove it from ALL
   the other passes that uses this same source list, which is not what we want
   (we still want them to be rendered on the main or shadow pass).

   Thus each draw list has two mesh list:
   1. the source mesh (const): which is the Source Of Truth, the actually list
   linked from thescene pipeline (Lit/ Unlit)
   2. the draw mesh: the more dynamic list from which we can enable or disable
   some meshes from the main list.

                      .----------.----------.----------.----------.----------.
   Scene Pipeline:    |  Mesh 1  |  Mesh 2  |  Mesh 3  |  Mesh 4  |  Mesh 5  |
                      '----------'----------'----------'----------'----------'
                                         └[ Linked ]┐
                      .----------.----------.----------.----------.----------.
   Source list:       |  Mesh 1  |  Mesh 2  |  Mesh 3  |  Mesh 4  |  Mesh 5  |
                      '-----.----'----------'----.----'-----.----'----------'
                            |          .---------'.---------'
                      .-----'----.-----'----.----'-----.
   Draw list:         | Packet 1 | Packet 3 | Packet 4 |
                      '----------'----------'----------'

   The caveats to this double layer list is that we need to make sure to sync
   the draw list when we add or remove mesh from the scene.

   ==========================================================================
   ==========================================================================

   Notice some of the interface function may be slower that others du to their
  query scope.

   Render pass list  ------------------- (level 1: slowest)
        L Render Pass ------------------ (level 2: mid)
             L Layout ------------------ (level 3: fastest)
                 L Mesh list
                      L Mesh



   .--- Render pass list ----------------------------------------------------.
   |                                                                         |
   |  .--- Render pass 1 ----------------.--- Render pass 2 --------------.  |
   |  |                                  |                                |  |
   |  | .--------.--------.--------.-----| .--------.--------.--------.---|  |
   |  | | Layout | Layout | Layout | Lay | | Layout | Layout | Layout | L |  |
   |  | '--------'--------'--------'-----| '--------'--------'--------'---|  |
   |  |                                  |                                |  |
   |  '----------------------------------'--------------------------------'  |
   |                                                                         |
   '-------------------------------------------------------------------------'

   As we can see, by targetting the layout directly we drastically reduce the
   query scope.

 */

/*
  Target the right render pass draw layout based on the provided ref list
  pointer
 */
static inline void
render_pass_find_layout_from_mesh(RenderPass *pass, const Mesh *mesh,
                                  RenderPassLayoutRefList *dest) {

  *dest = (RenderPassLayoutRefList){0};

  for (uint16_t i = 0; i < pass->stagged_list.count; i++) {
    const MeshRefList *ref_list = pass->stagged_list.entries[i].src_meshes;
    if (mesh_ref_list_find(ref_list, mesh, NULL) != NULL &&
        dest->count < RENDER_PASS_MAX_DRAW_LIST)
      dest->entries[dest->count++] = &pass->stagged_list.entries[i];
  }
}

static inline RenderPassLayout *
render_pass_find_layout_from_source_list(RenderPass *pass,
                                         const MeshRefList *source_list) {

  for (uint16_t i = 0; i < pass->stagged_list.count; i++)
    if (pass->stagged_list.entries[i].src_meshes == source_list)
      return &pass->stagged_list.entries[i];

  return NULL;
}

/**
             .============ LEVEL 3 ============.

   ▗▄▄▖ ▗▄▄▄▖▗▖  ▗▖▗▄▄▄ ▗▄▄▄▖▗▄▄▖     ▗▖    ▗▄▖▗▖  ▗▖▗▄▖ ▗▖ ▗▖▗▄▄▄▖
   ▐▌ ▐▌▐▌   ▐▛▚▖▐▌▐▌  █▐▌   ▐▌ ▐▌    ▐▌   ▐▌ ▐▌▝▚▞▘▐▌ ▐▌▐▌ ▐▌  █
   ▐▛▀▚▖▐▛▀▀▘▐▌ ▝▜▌▐▌  █▐▛▀▀▘▐▛▀▚▖    ▐▌   ▐▛▀▜▌ ▐▌ ▐▌ ▐▌▐▌ ▐▌  █
   ▐▌ ▐▌▐▙▄▄▖▐▌  ▐▌▐▙▄▄▀▐▙▄▄▖▐▌ ▐▌    ▐▙▄▄▖▐▌ ▐▌ ▐▌ ▝▚▄▞▘▝▚▄▞▘  █


 */

/**
   Insert the packet that has the mesh in the drawn list. Also prevents adding
   it twice.
 */
static inline RenderPassStatus
render_pass_layout_enable_mesh(RenderPassLayout *layout, Mesh *mesh) {

  // prevent adding it twice if already in it
  MeshDrawPacket *pack =
      mesh_draw_packet_list_find_mesh(&layout->drawn_meshes, mesh, NULL);

  if (pack)
    return RenderPassStatus_MeshAlreadyEnabled;

  pack = mesh_draw_packet_list_new_entry(&layout->drawn_meshes);

  if (pack)
    mesh_create_draw_packet(layout->topology_callback(mesh),
                            mesh_shader(mesh, layout->shader), mesh, pack);
  else
    return RenderPassStatus_DrawListUpdateError;

  return RenderPassStatus_Success;
}

/**
   Remove the packet that has the mesh from the drawn list.
 */
static inline RenderPassStatus
render_pass_layout_disable_mesh(RenderPassLayout *layout, Mesh *mesh) {

  size_t index;
  MeshDrawPacket *pack =
      mesh_draw_packet_list_find_mesh(&layout->drawn_meshes, mesh, &index);

  if (pack && mesh_draw_packet_list_remove_at_index(
                  &layout->drawn_meshes, index) == DynamicListStatus_Success) {
    return RenderPassStatus_Success;
  }

  return RenderPassStatus_DrawListUpdateError;
}

static inline RenderPassStatus
render_pass_layout_enable_all_mesh(RenderPassLayout *layout) {

  const MeshRefList *src_list = layout->src_meshes;
  MeshDrawPacketList *pack_list = &layout->drawn_meshes;

  mesh_draw_packet_list_empty(pack_list);

  for (size_t i = 0; i < src_list->count; i++) {
    Mesh *mesh = src_list->entries[i];
    MeshDrawPacket *pack = mesh_draw_packet_list_new_entry(pack_list);
    if (pack)
      mesh_create_draw_packet(layout->topology_callback(mesh),
                              mesh_shader(mesh, layout->shader), mesh, pack);
  }

  return RenderPassStatus_Success;
}

static inline RenderPassStatus
render_pass_layout_disable_all_mesh(RenderPassLayout *layout) {
  mesh_draw_packet_list_empty(&layout->drawn_meshes);
  return RenderPassStatus_Success;
}

/**
            .============ LEVEL 2 ============.

   ▗▄▄▖ ▗▄▄▄▖▗▖  ▗▖▗▄▄▄ ▗▄▄▄▖▗▄▄▖     ▗▄▄▖  ▗▄▖  ▗▄▄▖ ▗▄▄▖
   ▐▌ ▐▌▐▌   ▐▛▚▖▐▌▐▌  █▐▌   ▐▌ ▐▌    ▐▌ ▐▌▐▌ ▐▌▐▌   ▐▌
   ▐▛▀▚▖▐▛▀▀▘▐▌ ▝▜▌▐▌  █▐▛▀▀▘▐▛▀▚▖    ▐▛▀▘ ▐▛▀▜▌ ▝▀▚▖ ▝▀▚▖
   ▐▌ ▐▌▐▙▄▄▖▐▌  ▐▌▐▙▄▄▀▐▙▄▄▖▐▌ ▐▌    ▐▌   ▐▌ ▐▌▗▄▄▞▘▗▄▄▞▘


 */

static inline RenderPassStatus render_pass_enable_mesh(RenderPass *pass,
                                                       Mesh *mesh) {

  RenderPassLayoutRefList layout_list;
  render_pass_find_layout_from_mesh(pass, mesh, &layout_list);

  RenderPassStatus status = RenderPassStatus_Success;

  for (size_t i = 0; i < layout_list.count; i++) {

    RenderPassStatus enable =
        render_pass_layout_enable_mesh(layout_list.entries[i], mesh);
    if (enable != RenderPassStatus_Success)
      status = enable;
  }

  render_pass_sync_drawn_layouts(pass);

  return status;
}

static inline RenderPassStatus render_pass_disable_mesh(RenderPass *pass,
                                                        Mesh *mesh) {

  RenderPassLayoutRefList layout_list;
  render_pass_find_layout_from_mesh(pass, mesh, &layout_list);

  RenderPassStatus status = RenderPassStatus_Success;

  for (size_t i = 0; i < layout_list.count; i++) {

    RenderPassStatus disable =
        render_pass_layout_disable_mesh(layout_list.entries[i], mesh);

    if (disable != RenderPassStatus_Success)
      status = disable;
  }

  render_pass_sync_drawn_layouts(pass);

  return status;
}

static inline RenderPassStatus
render_pass_enable_mesh_ref_list(RenderPass *pass, MeshRefList *meshes) {

  for (size_t i = 0; i < meshes->count; i++)
    render_pass_enable_mesh(pass, meshes->entries[i]);

  return RenderPassStatus_Success;
}

static inline RenderPassStatus
render_pass_disable_mesh_ref_list(RenderPass *pass, MeshRefList *meshes) {

  for (size_t i = 0; i < meshes->count; i++)
    render_pass_disable_mesh(pass, meshes->entries[i]);

  return RenderPassStatus_Success;
}

static inline RenderPassStatus render_pass_enable_all_mesh(RenderPass *pass) {

  for (uint16_t i = 0; i < pass->stagged_list.count; i++) {
    RenderPassLayout *layout = &pass->stagged_list.entries[i];
    render_pass_layout_enable_all_mesh(layout);
  }

  return RenderPassStatus_Success;
}

static inline RenderPassStatus render_pass_disable_all_mesh(RenderPass *pass) {

  for (uint16_t i = 0; i < pass->stagged_list.count; i++) {
    RenderPassLayout *layout = &pass->stagged_list.entries[i];
    render_pass_layout_disable_all_mesh(layout);
  }

  return RenderPassStatus_Success;
}

/**
           .============ LEVEL 1 ============.

   ▗▄▄▖ ▗▄▄▄▖▗▖  ▗▖▗▄▄▄ ▗▄▄▄▖▗▄▄▖     ▗▖   ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖
   ▐▌ ▐▌▐▌   ▐▛▚▖▐▌▐▌  █▐▌   ▐▌ ▐▌    ▐▌     █  ▐▌     █
   ▐▛▀▚▖▐▛▀▀▘▐▌ ▝▜▌▐▌  █▐▛▀▀▘▐▛▀▚▖    ▐▌     █   ▝▀▚▖  █
   ▐▌ ▐▌▐▙▄▄▖▐▌  ▐▌▐▙▄▄▀▐▙▄▄▖▐▌ ▐▌    ▐▙▄▄▖▗▄█▄▖▗▄▄▞▘  █

 */

static inline RenderPassStatus
render_pass_list_enable_mesh(RenderPassList *list, Mesh *mesh) {

  for (uint8_t i = 0; i < list->count; i++)
    render_pass_enable_mesh(&list->passes[i], mesh);

  return RenderPassStatus_Success;
}

static inline RenderPassStatus
render_pass_list_disable_mesh(RenderPassList *list, Mesh *mesh) {
  for (uint8_t i = 0; i < list->count; i++)
    render_pass_disable_mesh(&list->passes[i], mesh);

  return RenderPassStatus_Success;
}

static inline RenderPassStatus
render_pass_list_enable_mesh_ref_list(RenderPassList *list,
                                      MeshRefList *meshes) {

  for (uint8_t i = 0; i < list->count; i++)
    for (size_t j = 0; j < meshes->count; j++)
      render_pass_enable_mesh(&list->passes[i], meshes->entries[j]);

  return RenderPassStatus_Success;
}

static inline RenderPassStatus
render_pass_list_disable_mesh_ref_list(RenderPassList *list,
                                       MeshRefList *meshes) {

  for (uint8_t i = 0; i < list->count; i++)
    for (size_t j = 0; j < meshes->count; j++)
      render_pass_disable_mesh(&list->passes[i], meshes->entries[j]);

  return RenderPassStatus_Success;
}

static inline RenderPassStatus
render_pass_list_disable_all_mesh(RenderPassList *list) {

  for (uint16_t i = 0; i < list->count; i++)
    render_pass_disable_all_mesh(&list->passes[i]);

  return RenderPassStatus_Success;
}

static inline RenderPassStatus
render_pass_list_enable_all_mesh(RenderPassList *list) {

  for (uint16_t i = 0; i < list->count; i++)
    render_pass_enable_all_mesh(&list->passes[i]);

  return RenderPassStatus_Success;
}

#endif
