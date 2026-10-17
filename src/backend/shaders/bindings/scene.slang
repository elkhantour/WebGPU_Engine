module scene;
#include "include/bind_layouts.h"
import core.types;

// Scene bind group (see include/bind_layouts.h). Importing this module is all
// a world-space pass needs. Unused resources are dropped from the WGSL.
[[vk::binding(SCENE_VIEWPORT_BINDING, SCENE_GROUP)]] public ConstantBuffer<Viewport> uViewport;
[[vk::binding(SCENE_CAMERA_BINDING,   SCENE_GROUP)]] public ConstantBuffer<Camera>   uCamera;
[[vk::binding(SCENE_MESH_BINDING,     SCENE_GROUP)]] public ConstantBuffer<Mesh>     uMesh;
