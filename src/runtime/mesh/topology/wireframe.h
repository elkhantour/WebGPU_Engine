#ifndef _MESH_TOPOLOGY_WIREFRAME_H_
#define _MESH_TOPOLOGY_WIREFRAME_H_

#include <webgpu/webgpu.h>

#include "anchor.h"
#include "base.h"
#include "core.h"
#include "runtime/geometry/vertex/attribute.h"
#include "runtime/geometry/vertex/index.h"
#include "runtime/geometry/vertex/vertex.h"

typedef enum {
  MeshTopologyWireframeStatus_Success,
  MeshTopologyWireframeStatus_AllocFail,
  MeshTopologyWireframeStatus_UnderError,
} MeshTopologyWireframeStatus;

typedef struct {
  VertexIndex index;
  VertexAttribute attribute;
  MeshTopologyAnchorList anchors;
} MeshTopologyWireframe;

EXTERN_C_BEGIN

MeshTopologyWireframeStatus
mesh_topology_wireframe_create(MeshTopology *, MeshTopologyWireframe *);

MeshTopologyWireframeStatus
mesh_topology_wireframe_upload(MeshTopologyWireframe *);

MeshTopology mesh_topology_wireframe_vertex(MeshTopologyWireframe *);

int mesh_topology_wireframe_update(const MeshTopologyBase *,
                                   MeshTopologyWireframe *);

EXTERN_C_END
#endif
