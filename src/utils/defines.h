#ifndef _UTILS_DEFINES_H_
#define _UTILS_DEFINES_H_

#ifdef __cplusplus
  #define EXTERN_C_BEGIN extern "C" {
  #define EXTERN_C_END   }
#else
  #define EXTERN_C_BEGIN
  #define EXTERN_C_END
#endif


#define ENGINE_PREFIX

// If adjusting those, make sure to sync with projects CMake resources mapping
#define RESOURCES_PATH_SHADER(filename) "./shaders/" filename
#define RESOURCES_PATH_TEXTURE(filename) "./textures/" filename
#define RESOURCES_PATH_FONT(filename) "./fonts/" filename
#define RESOURCES_PATH_GLTF(filename) "./gltf/" filename
#define RESOURCES_PATH_MBIN(filename) "./mbin/" filename




#endif
