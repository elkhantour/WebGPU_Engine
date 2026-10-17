#ifndef _STD_PIPELINE_MODULES_COMMONS_H_
#define _STD_PIPELINE_MODULES_COMMONS_H_

#include "runtime/camera/camera.h"
#include "runtime/mesh/mesh.h"
#include "runtime/viewport/viewport.h"
#include "utils/projection.h"

#include "webgpu/webgpu.h"

static const WGPUBindGroupLayoutEntry mvp_layout_entry[3] = {
    {
        .binding = 0,
        .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
        .buffer =
            (WGPUBufferBindingLayout){
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = true,
                .minBindingSize = sizeof(ViewportUniform),
            },
    },
    {
        .binding = 1,
        .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
        .buffer =
            (WGPUBufferBindingLayout){
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = true,
                .minBindingSize = sizeof(CameraUniform),
            },
    },
    {
        .binding = 2,
        .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
        .buffer =
            (WGPUBufferBindingLayout){
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = true,
                .minBindingSize = sizeof(MeshUniform),
            },
    },
};

static const WGPUBindGroupLayoutDescriptor mvp_layout = {
    // Group 0
    .label = "Group 0 (Scene Data)",
    .entryCount = 3,
    .entries = mvp_layout_entry,
};

static const PipelineBindingMVP mvp_binding = {
    .group = 0,
    .projection = 0,
    .view = 1,
    .model = 2,
};

static const WGPUBindGroupLayoutDescriptor mp_layout = {
    // Group 0: view_projection + uModel
    .label = "Group 0 - ViewProj Model Matrices",
    .entryCount = 2,
    .entries =
        (WGPUBindGroupLayoutEntry[]){
            {
                // view_projection
                .binding = 0,
                .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
                .buffer =
                    (WGPUBufferBindingLayout){
                        .type = WGPUBufferBindingType_Uniform,
                        .hasDynamicOffset = true,
                        .minBindingSize = sizeof(ProjectionUniform),
                    },
            },
            {
                // uModel
                .binding = 1,
                .visibility = WGPUShaderStage_Vertex,
                .buffer =
                    (WGPUBufferBindingLayout){
                        .type = WGPUBufferBindingType_Uniform,
                        .hasDynamicOffset = true,
                        .minBindingSize = sizeof(MeshUniform),
                    },
            },
        },
};

static const WGPUBlendState blend_alpha = {
    .color =
        {
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            .operation = WGPUBlendOperation_Add,
        },
    .alpha =
        {
            .srcFactor = WGPUBlendFactor_One,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
            .operation = WGPUBlendOperation_Add,
        },
};


static const WGPUPrimitiveState primitive_double_sided = {
    .frontFace = WGPUFrontFace_CCW,
    .cullMode = WGPUCullMode_None,
    .topology = WGPUPrimitiveTopology_TriangleList,
    .stripIndexFormat = WGPUIndexFormat_Undefined,
};


#endif
