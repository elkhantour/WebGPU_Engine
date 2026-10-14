#ifndef _UTILS_ID_H_
#define _UTILS_ID_H_

#include "logger.h"
#define REG_MAX_OBJECTS 2048
#define REG_OWNER_UNDEFINED -1

// clang-format off

#define REG_ENTRIES(_)                                                         \
  _(Mesh)                                                                      \
  _(Camera)                                                                    \
  _(AmbientLight)                                                              \
  _(PointLight)                                                                \
  _(SpotLight)                                                                 \
  _(SunLight)                                                                  \
  _(Scene)                                                                     \
  _(Gui)                                                                       \
  _(SceneLayer)                                                                \
  _(ProbeReflectionPlane)                                                      \
  _(ProbeReflection)                                                           \
  _(ProbeReflectionGrid)                                                       \
  _(SceneEditorMesh)                                                           \
  _(SceneEditorMeshList)                                                       \
  _(SceneEditorMeshList_PointLight)                                            \
  _(SceneEditorMeshList_AmbientLight)                                          \
  _(SceneEditorMeshList_SunLight)                                              \
  _(SceneEditorMeshList_SpotLight)                                             \
  _(SceneEditorMeshList_PointLightShadow)                                      \
  _(SceneEditorMeshList_SunLightShadow)                                        \
  _(SceneEditorMeshList_SpotLightShadow)                                       \
  _(SceneEditorMeshList_Camera)                                                \
  _(SceneEditorMeshList_ProbeReflectionPlane)                                  \
  _(SceneEditorMeshList_ProbeReflectionGrid)                                   \
  _(RenderPipeline)                                                            \
  _(ComputePipeline)                                                           \
  _(Texture)                                                                   \
  _(Shader)                                                                    \
  _(Ubo)                                                                       \
  _(JobSystem)                                                                 \
  _(Renderer)                                                                  \
  _(WGPUObject)

// clang-format on

typedef int reg_id_t;
typedef enum {
#define _(Name) RegEntryType_##Name,
  REG_ENTRIES(_)
#undef _
} RegEntryType;

static const char *reg_label[] = {
#define _(Name) [RegEntryType_##Name] = #Name,
    REG_ENTRIES(_)
#undef _
};

typedef struct {
  reg_id_t id;
  RegEntryType type;
  void *ptr;
} RegEntry;

EXTERN_C_BEGIN

// TODO: maybe segment reg into per-type, so it's faster to access the elements
// of a certain type RegEntry g_reg[TYPE_COUNT][REG_MAX_OBJECTS]
extern RegEntry g_reg[REG_MAX_OBJECTS];
extern reg_id_t g_reg_id;

static inline reg_id_t reg_new_id() {
  if (g_reg_id == REG_MAX_OBJECTS) {
    logger_add(LoggerFlag_Error, "Cannot add more objects to registry.");
    return REG_MAX_OBJECTS;
  }

  return g_reg_id++;
}

reg_id_t reg_register(void *, RegEntryType);

static inline const int reg_count() { return g_reg_id; }

static inline const RegEntry *reg_lookup(reg_id_t id) {
  if (id >= REG_MAX_OBJECTS) {
    logger_add(LoggerFlag_Error, "id out of registry bounds.");
    return NULL;
  }

  return &g_reg[id];
}

static inline size_t reg_type_count(const RegEntryType type) {
  size_t count = 0;
  for (size_t i = 0; i < g_reg_id; i++)
    if (g_reg[i].type == type)
      count++;

  return count;
}

EXTERN_C_END

#endif
