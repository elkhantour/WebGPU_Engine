#include "skybox.h"

#include "../include/prefab.h"

static const CubeMapPath lake_cubemap = {
    .right = RESOURCES_PATH_TEXTURE(skybox/lake/right.jpg),
    .left = RESOURCES_PATH_TEXTURE(skybox/lake/left.jpg),
    .top = RESOURCES_PATH_TEXTURE(skybox/lake/top.jpg),
    .bottom = RESOURCES_PATH_TEXTURE(skybox/lake/bottom.jpg),
    .front = RESOURCES_PATH_TEXTURE(skybox/lake/front.jpg),
    .back = RESOURCES_PATH_TEXTURE(skybox/lake/back.jpg),
};


/**
   Create a picture based skybox in the given scene
 */
void example_skybox(Engine *engine) {
  prefab_skybox_create(engine, &(PrefabSkyboxCreateDescriptor){
                                   .blur = 4,
                                   .resolution = TextureResolution_512,
                                   .path = lake_cubemap,
                               });
}

/**
   Create a gradient skybox in the given scene
 */
void example_skybox_gradient(Engine *engine) {

  prefab_skybox_gradient_create(
      engine, &(PrefabSkyboxGradientCreateDescriptor){
                  .resolution = 32,
                  .stops =
                      {
                          .count = 2,
                          .capacity = 2,
                          .entries =
                              (TextureGradientStop[]){
                                  {
                                      .color = (uint8_t[]){240, 245, 255, 255},
                                      .position = 1.0f,
                                  },
                                  {
                                      .color = (uint8_t[]){51, 153, 255, 255},
                                      .position = 0.0f,
                                  },
                              },
                      },
              });
}
