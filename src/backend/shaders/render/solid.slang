import core.types;
import core.transform;
import bindings.scene;
import lighting.lights;

struct VertexOut
{
    float4 vPosition : SV_Position;
    [[vk::location(0)]] float3 vNormal : NORMAL;
    [[vk::location(1)]] float3 vCol    : COLOR;
    [[vk::location(2)]] float2 vUv     : TEXCOORD0;
    [[vk::location(3)]] float3 vFrag   : TEXCOORD1;
};

[shader("vertex")]
VertexOut vs_main(VertexIn input)
{
    float4 world = local_to_world(uMesh, input.aPos);

    VertexOut output;
    output.vPosition = world_to_clip(uViewport, uCamera, world);
    output.vNormal = normalize(input.aNorm);
    output.vCol = input.aCol;
    output.vUv  = input.aUv;
    // world-space position, used for lighting
    output.vFrag = world.xyz;
    return output;
}

// Simple N.L term against a light position
float3 compute_lambert(Mesh mesh, float3 fragment_position, float3 vertex_normal,
                       float3 light_position)
{
    float3 world_normal = mul(mesh.model, float4(vertex_normal, 0.0)).xyz;
    return float3(dot(normalize(world_normal), normalize(light_position - fragment_position)));
}

[shader("fragment")]
float4 fs_main(VertexOut input) : SV_Target
{
    Mesh mesh = uMesh;

    SunLight sun_light;
    sun_light.position  = float3(20.0, 20.0, 10.0);
    sun_light.intensity = 0.2;
    sun_light.color     = float4(1.0, 1.0, 1.0, 1.0);

    float3 Lo = compute_lambert(mesh, input.vFrag, input.vNormal, sun_light.position);

    float shadow = 0.4;
    float light  = 0.7;
    Lo = lerp(float3(shadow), float3(light), Lo);
    return float4(Lo, 1.0);
}
