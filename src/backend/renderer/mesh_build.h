#ifndef _RENDERER_MESH_BUILD_H_
#define _RENDERER_MESH_BUILD_H_

#include "backend/job_system.h"
#include "backend/renderer/core.h"
#include "backend/renderer/mesh_config.h"
#include "runtime/scene/core.h"

/*
   Mesh Binding process:

   The scene building process handles each layers respective essentials shader
   creation or binding process( view matrix...).

   Currently the renderer handles different passes such as :
   - Topology Creation
   - Shader creation
   - Shader bind views
   - Shader bind lights
   - Shader build pipeline layout

   .---------------------------------------------------------------------.
   |                ADD               |              REMOVE              |
   |---------------------------------------------------------------------|
   |   Add and Remove functions basically mounts, unmounts the mesh      |
   |   from the scene. Meaning they build the mesh shader internally     |
   |   and show it visually by adding it to the pipeline list.           |
   |   Those 2 functions should only be used for first and last instan-  |
   |   tiation of the mesh.                                              |
   |                                                                     |
   |   .------------ ⚙ ------------.    .------------ ◉ -------------.  |
   |   |    BUILD    |   UNBUILD    |    |     SHOW    |     HIDE     |  |
   |   |----------------------------|    |----------------------------|  |
   |   | Build and Unbuild function | => | Show and Hide functions    |  |
   |   | only handle the mesh       | => | operate at a visual level  |  |
   |   | internal binding. It does  | => | only. They only pop or push|  |
   |   | not visually add the mesh  | => | the mesh from the pipeline |  |
   |   | to the scene pipeline.     | => | array. However it's        |  |
   |   | Building only "prepares"   | => | important to make sure the |  |
   |   | the mesh for the drawcall. |    | mesh is Built priorly.     |  |
   |   '----------------------------'    '----------------------------'  |
   '---------------------------------------------------------------------'

 */

RendererStatus renderer_build_mesh(Renderer *, Scene *, JobSystem *, Mesh *,
                                   const RendererMeshFlag);

#endif
