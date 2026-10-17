import core.types;
import bindings.scene;

[[vk::binding(0, 1)]]
ConstantBuffer<float4> uColor;
[[vk::binding(1, 1)]]
ConstantBuffer<float> uFixedScale;

struct VertexOut {
  float4 Position : SV_Position;
  [[vk::location(0)]]
  float3 vCol : COLOR;
};

[shader("vertex")]
VertexOut vs_main(VertexIn input) {
  Mesh mesh = uMesh;
  Camera camera = uCamera;
  Viewport viewport = uViewport;

  float fixed_scale = uFixedScale;

  float4 world_pos = mul(mesh.model, float4(input.aPos, 1.0));

  float4 cam_to_mesh = normalize(mesh.position - camera.position);
  float4 fixed_origin = camera.position + cam_to_mesh * fixed_scale;
  float4 offset = world_pos - mesh.position;

  // 0 = world position; 1 = fixed position
  float4 blend_pos =
      lerp(world_pos, fixed_origin + offset, clamp(fixed_scale, 0.0, 1.0));

  VertexOut output;
  output.Position = mul(viewport.projection, mul(camera.view, blend_pos));
  output.vCol = input.aCol;
  return output;
}

// NOTE: unlit.wgsl had no fragment shader (it was used for vertex-only passes
// or the color came from elsewhere). Add one here if needed, e.g.:
[shader("fragment")]
float4 fs_main(VertexOut i) : SV_Target { return uColor; }
