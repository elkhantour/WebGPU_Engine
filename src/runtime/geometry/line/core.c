#include "core.h"

#include <cglm/vec4.h>
#include <stdbool.h>
#include <stdlib.h>

#include "backend/logger.h"
#include "backend/shaders/core.h"
#include "runtime/geometry/vertex/attribute.h"
#include "runtime/geometry/vertex/core.h"
#include "runtime/geometry/vertex/index.h"
#include "runtime/mesh/core.h"
#include "runtime/mesh/shader/core.h"
#include "runtime/mesh/topology/base.h"
#include "runtime/pipeline/render.h"
#include "runtime/shader/core.h"
#include "string.h"

static void line_create_plane(const LineCreatePlaneDescriptor *);

void line_create(Mesh *mesh, const LineCreateDescriptor *desc) {

  mesh_create(mesh, &(MeshCreateDescriptor){
                        .vertex = (VertexAttribute){0},
                        .index = (VertexIndex){0},
                        .name = desc->name,
                    });

  // create vertex attributes
  size_t attribute_capacity =
      LINE_MAX_POINTS * VERTEX_STRIDE * LINE_VERTEX_COUNT;
  vattr_t *vertex_attributes = calloc(attribute_capacity, sizeof(vattr_t));

  mesh_topology_base_create_vertex_attribute(&mesh->topology.base,
                                             &(VertexAttribute){
                                                 .entries = vertex_attributes,
                                                 .count = 0,
                                                 .capacity = attribute_capacity,
                                             });

  // crate vertex index
  size_t index_capacity = LINE_MAX_POINTS * LINE_INDEX_COUNT;
  vindex_t *vertex_index = calloc(index_capacity, sizeof(vindex_t));

  mesh_topology_base_create_vertex_index(&mesh->topology.base,
                                         &(VertexIndex){
                                             .entries = vertex_index,
                                             .count = 0,
                                             .capacity = index_capacity,
                                         });

  mesh_shader_create(
      mesh, &(ShaderCreateDescriptor){
                .pipeline = std_render_pipeline(RenderPipelineType_Line),
                .name = "Line",
            });
}

/** Define vertex data from a vertex array.
    Use different structure as casual vertex:

    Position => Position A
    Normal   => Position Ā
    Color    => Color
    UV       => [0] Extrude Direction, [1] Thickness
 */
void line_set_vertex(const vec3 base, const vec3 opposite, const vec4 color,
                     const vec2 extra, const size_t offset, float *data) {

  // set position
  memcpy(&data[offset + VertexAttributeOffset_Position], base,
         sizeof(vertex_position));

  // set normal
  memcpy(&data[offset + VertexAttributeOffset_Normal], opposite,
         sizeof(vertex_normal));

  // set tengant (unused)
  memcpy(&data[offset + VertexAttributeOffset_Tangent], GLM_VEC4_ZERO,
         sizeof(vertex_tangent));

  // set color
  memcpy(&data[offset + VertexAttributeOffset_Color], color,
         sizeof(vertex_color));

  // set UV
  memcpy(&data[offset + VertexAttributeOffset_Uv], extra, sizeof(vertex_uv));
}

void line_create_plane(const LineCreatePlaneDescriptor *desc) {

  /*

          Extrude along cross product w/ up vector
         ------------->

       A (p1)           B (p1-thickness)
        +--------------+
        ||'-           |
        ||  '_         |
        ||    '-       |
        ||      '-     |
        ||        '-   |
        ||          '- |
        +--------------+
       D (p2)	        C (p2-thickness)

   */

  size_t vertex_offset = desc->vertex->count;
  size_t vertex_count = 4;
  vec3 normal = {0.0f, 1.0f, 0.0f};
  vec2 uv = {0.0f, 0.0f};
  // add points to the array contiguously

  // A (p1)
  line_set_vertex(desc->points[0], normal, desc->color, uv, vertex_offset,
                  desc->vertex->entries);

  // B (p1 thickness)
  line_set_vertex(desc->points[1], normal, desc->color, uv,
                  vertex_offset + VERTEX_STRIDE, desc->vertex->entries);

  // C (p2 thickness)
  line_set_vertex(desc->points[2], normal, desc->color, uv,
                  vertex_offset + 2 * VERTEX_STRIDE, desc->vertex->entries);

  // D (p2)
  line_set_vertex(desc->points[3], normal, desc->color, uv,
                  vertex_offset + 3 * VERTEX_STRIDE, desc->vertex->entries);

  // add indices (A-B-C & A-C-D)
  size_t vertex_length = desc->vertex->count / VERTEX_STRIDE;

  desc->index->entries[desc->index->count] = (vindex_t)vertex_length;
  desc->index->entries[desc->index->count + 1] = (vindex_t)vertex_length + 1;
  desc->index->entries[desc->index->count + 2] = (vindex_t)vertex_length + 2;
  desc->index->entries[desc->index->count + 3] = (vindex_t)vertex_length + 2;
  desc->index->entries[desc->index->count + 4] = (vindex_t)vertex_length + 3;
  desc->index->entries[desc->index->count + 5] = (vindex_t)vertex_length;

  // update index count
  desc->index->count += 6;

  // update vertex count
  desc->vertex->count += vertex_count * VERTEX_STRIDE;
}

/**
   Add to vertices of specific color to the mesh vertex and indice array.
   Actually drawing a very very thin polygon so n points will:
   1. allocate 2n * vertex_stride in the vertex data
   2. 2n in the index
 */
void line_add_point(vec3 p1, vec3 p2, vec3 color,
                    VertexAttribute *vertex_attribute,
                    VertexIndex *vertex_index) {

  if (vertex_attribute->count == vertex_attribute->capacity) {
    logger_add(LoggerFlag_Warning,
               "Line vertex attribute reached max capacity.");
    return;
  }

  // update vertex array
  for (int p = 0; p < LINE_VERTEX_COUNT; p++) {
    bool isB = p > 0 && p < 3;

    float *base = isB ? p2 : p1;
    float *opposite = isB ? p1 : p2;
    float dir_mul = isB ? 1.0f : -1.0f;

    float side = (p < 2) ? 1.0f : -1.0f;

    line_set_vertex(base, opposite, color, (vec2){side, dir_mul},
                    vertex_attribute->count, vertex_attribute->entries);

    vertex_attribute->count += VERTEX_STRIDE;
  }

  // update index array
  size_t vertex_count =
      (vertex_attribute->count / VERTEX_STRIDE) - LINE_VERTEX_COUNT;

  vertex_index->entries[vertex_index->count] = (vindex_t)vertex_count;
  vertex_index->entries[vertex_index->count + 1] = (vindex_t)vertex_count + 1;
  vertex_index->entries[vertex_index->count + 2] = (vindex_t)vertex_count + 2;
  vertex_index->entries[vertex_index->count + 3] = (vindex_t)vertex_count;
  vertex_index->entries[vertex_index->count + 4] = (vindex_t)vertex_count + 2;
  vertex_index->entries[vertex_index->count + 5] = (vindex_t)vertex_count + 3;

  vertex_index->count += 6;
}

void line_update_buffer(Mesh *mesh) {
  // update mesh vertex + index buffers
  mesh_topology_base_update_buffer(&mesh->topology.base);
}
