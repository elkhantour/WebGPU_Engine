/* ---------------------------------------------------------------------------
 * Bind group layouts shared by shaders AND the C engine.
 *
 * Pure #defines: valid C and valid Slang (Slang runs the C preprocessor).
 * Include it from the engine when creating WGPUBindGroupLayout /
 * WGPUBindGroupEntry, and from the Slang binding modules.
 *
 * Rule: a bind group used by 2+ shaders is described HERE. Bind groups that
 * belong to a single shader stay as literals in that shader's file.
 * ------------------------------------------------------------------------- */
#ifndef BIND_LAYOUTS_H
#define BIND_LAYOUTS_H

/* ---- scene: every world-space render pass --------------------------------
 *   viewport / camera / mesh uniforms                                       */
#define SCENE_GROUP              0
#define SCENE_VIEWPORT_BINDING   0   /* uniform  Viewport */
#define SCENE_CAMERA_BINDING     1   /* uniform  Camera   */
#define SCENE_MESH_BINDING       2   /* uniform  Mesh     */

/* ---- image filter: every compute pass that maps one texture to another ---
 *   src texture + sampler -> dst storage texture (rgba8unorm, write)
 *   binding 3 is an OPTIONAL per-filter uniform (colormap, kawase, ...)     */
#define IMAGE_FILTER_GROUP               0
#define IMAGE_FILTER_SRC_BINDING         0   /* texture_2d<f32>            */
#define IMAGE_FILTER_SAMPLER_BINDING     1   /* sampler                    */
#define IMAGE_FILTER_DST_BINDING         2   /* texture_storage_2d, write  */
#define IMAGE_FILTER_PARAMS_BINDING      3   /* uniform (optional)         */

/* workgroup is TILE x TILE; dispatch ceil(width/TILE) x ceil(height/TILE)   */
#define IMAGE_FILTER_TILE_SIZE           8

#endif /* BIND_LAYOUTS_H */
