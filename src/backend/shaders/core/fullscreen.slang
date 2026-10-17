module fullscreen;

// Fullscreen triangle, no vertex buffer needed: draw(3).
// https://stackoverflow.com/questions/2588875/whats-the-best-way-to-draw-a-fullscreen-quad-in-opengl-3-2
public struct FullscreenOut
{
    public float4 Position : SV_Position;
    [[vk::location(0)]] public float2 vUv : TEXCOORD0;
};

public FullscreenOut fullscreen_triangle(uint vertex_index)
{
    static const float2 vertices[3] = {
        float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0, 3.0)
    };

    FullscreenOut output;
    output.Position = float4(vertices[vertex_index], 0.0, 1.0);
    output.vUv = float2(0.5, -0.5) * output.Position.xy + float2(0.5, 0.5);
    return output;
}
