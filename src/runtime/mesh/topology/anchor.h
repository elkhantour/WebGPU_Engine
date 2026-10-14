#ifndef _MESH_TOPOLOGY_ANCHOR_H_
#define _MESH_TOPOLOGY_ANCHOR_H_

#include <stddef.h>
#include <stdint.h>

#include "core.h"
#include "runtime/geometry/vertex/attribute.h"
#include "runtime/geometry/vertex/core.h"
#include "runtime/geometry/vertex/index.h"
#include "utils/defines.h"
#include "utils/dyli.h"

#define MESH_TOPOLOGY_ANCHOR_LIST_DEFAULT_CAPACITY 1023
#define MESH_TOPOLOGY_ANCHOR_DEFAULT_CAPACITY 32

typedef enum {
  MeshTopologyAnchorStatus_Success,
  MeshTopologyAnchorStatus_AllocFail,
  MeshTopologyAnchorStatus_Unset,
  MeshTopologyAnchorStatus_UndefError,
} MeshTopologyAnchorStatus;

typedef union {
  vattr_t f;
  uint32_t u;
} MeshTopologyAnchorKey;

typedef DYNAMIC_LIST(vindex_t) MeshTopologyAnchor;
typedef DYNAMIC_LIST(MeshTopologyAnchor) MeshTopologyAnchorList;

EXTERN_C_BEGIN

/**
   Anchors provide implementation to help with topology manipulation.
   An anchor is simply a index list that is linked to an index.
   The anchor system basically helps to copy one vertex transform to it's
   children. As instance with anchors, by editing the vertex 0 on base topology
   we can also affect all the linked wireframe instance.
 */

/**
     ▗▄▖ ▗▖  ▗▖ ▗▄▄▖▗▖ ▗▖ ▗▄▖ ▗▄▄▖
    ▐▌ ▐▌▐▛▚▖▐▌▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌
    ▐▛▀▜▌▐▌ ▝▜▌▐▌   ▐▛▀▜▌▐▌ ▐▌▐▛▀▚▖
    ▐▌ ▐▌▐▌  ▐▌▝▚▄▄▖▐▌ ▐▌▝▚▄▞▘▐▌ ▐▌

 */
int mesh_topology_anchor_create(MeshTopologyAnchor *, size_t);
int mesh_topology_anchor_expand(MeshTopologyAnchor *);
int mesh_topology_anchor_insert(MeshTopologyAnchor *, vindex_t *, size_t);
void mesh_topology_anchor_print(MeshTopologyAnchor *);
void mesh_topology_anchor_merge(const MeshTopologyAnchorList *,
                                const vindex_t *, const size_t,
                                MeshTopologyAnchor *);

/**
     ▗▄▖ ▗▖  ▗▖ ▗▄▄▖▗▖ ▗▖ ▗▄▖ ▗▄▄▖     ▗▖   ▗▄▄▄▖ ▗▄▄▖▗▄▄▄▖
    ▐▌ ▐▌▐▛▚▖▐▌▐▌   ▐▌ ▐▌▐▌ ▐▌▐▌ ▐▌    ▐▌     █  ▐▌     █
    ▐▛▀▜▌▐▌ ▝▜▌▐▌   ▐▛▀▜▌▐▌ ▐▌▐▛▀▚▖    ▐▌     █   ▝▀▚▖  █
    ▐▌ ▐▌▐▌  ▐▌▝▚▄▄▖▐▌ ▐▌▝▚▄▞▘▐▌ ▐▌    ▐▙▄▄▖▗▄█▄▖▗▄▄▞▘  █

 */
uint32_t mesh_topology_anchor_list_hash_float(float);
uint32_t mesh_topology_anchor_list_hash(vertex_position);
int mesh_topology_anchor_list_expand(MeshTopologyAnchorList *);
int mesh_topology_anchor_list_create(MeshTopologyAnchorList *, size_t);

int mesh_topology_anchor_list_insert(MeshTopologyAnchorList *,
                                     vertex_position *, vindex_t *, size_t);

void mesh_topology_anchor_list_map(MeshTopologyAnchorList *, MeshTopology *,
                                   MeshTopologyAnchorList *);

// hash based query
MeshTopologyAnchor *mesh_topology_anchor_list_new_hash(MeshTopologyAnchorList *,
                                                       vertex_position *);

MeshTopologyAnchor *
mesh_topology_anchor_list_find_hash(MeshTopologyAnchorList *,
                                    vertex_position *);
// index based query
MeshTopologyAnchor *
mesh_topology_anchor_list_new_index(MeshTopologyAnchorList *, vindex_t);

void mesh_topology_anchor_list_destroy(MeshTopologyAnchorList *);

EXTERN_C_END
#endif
