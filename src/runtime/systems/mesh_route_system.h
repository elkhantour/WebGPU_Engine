#ifndef _MESH_ROUTE_SYSTEM_H_
#define _MESH_ROUTE_SYSTEM_H_

#include "backend/renderer/core.h"
#include "backend/renderer/mesh_config.h"
#include "runtime/scene/core.h"
#include "utils/defines.h"

/*

  The router helps to:
  1. Dispatch the scene meshes into the right render pass
  according to their rendering configuration (flags, layers, pipeline).
  Meaning if a mesh needs to be hidden, the router target the render passes the
  mesh is and removes it from them.

  2. Update and refresh the Scene Mesh Cache when a mesh rendering property
  changes (visiblity, cast shadow...). The Scene Mesh Cache store the meshes
  pointer into arrays with for each respective flags, pipeline or layer:

  [RenderFlag_Shadow] : [ Mesh*, Mesh*, Mesh* ]

  The Scene Cache is usefull to  quickly retrieve meshes with a certain type of
  flags (shadow, reflection, selection etc..). Else we would need to traverse
  and filter the whole list everytime.

                                                            .------------.
   .-Mesh Config-.                                     .--> | Render Pass |
   |  + flags    |           .----------.             |     '-------------'
   |  + pipeline | --(+/-)-> |  ROUTER  | --(update)- +
   |  + layer    |           '----------'             |     .------------.
   '-------------'                                    '--> | Scene Cache |
                                                          '-------------'


 */

EXTERN_C_BEGIN

// Show / hide: set or clear RendererMeshFlag_Hidden, then
// refresh the scene cache, then route the mesh, then update the counts.
void mesh_route_system_show_mesh(Scene *, Renderer *, Mesh *);
void mesh_route_system_hide_mesh(Scene *, Renderer *, Mesh *);

// Same, but the cache is refreshed and the counts updated once for the list.
void mesh_route_system_show_mesh_ref_list(Scene *, Renderer *, MeshRefList *);
void mesh_route_system_hide_mesh_ref_list(Scene *, Renderer *, MeshRefList *);

RendererStatus mesh_route_system_toggle_mesh(Scene *, Renderer *, Mesh *);

// Call after ANY change to a mesh's render_config (added to the scene,
// selection changed, layer/pipeline changed...): refresh cache + route.
void mesh_route_system_update_mesh(Scene *, Renderer *, Mesh *);
void mesh_route_system_update_mesh_ref_list(Scene *, Renderer *, MeshRefList *);

// Rebuild in place the draw packets of a mesh whose vertex/index buffers or
// shader were recreated. Call at a sync point, never during the draw loop.
void mesh_route_system_refresh_mesh(Renderer *, Mesh *);

// Remove the mesh from every layout (call before the mesh is destroyed).
void mesh_route_system_unroute_mesh(Scene *, Renderer *, Mesh *);

EXTERN_C_END

#endif
