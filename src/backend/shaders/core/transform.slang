module transform;
import core.types;

// Column i of a matrix (WGSL: `m[i]`; in Slang `m[i]` would be a ROW).
public float3 mat_column3(float4x4 m, int c)
{
    return float3(m[0][c], m[1][c], m[2][c]);
}

// Per-axis scale of a model matrix.
public float3 model_scale(float4x4 model)
{
    return float3(length(mat_column3(model, 0)),
                  length(mat_column3(model, 1)),
                  length(mat_column3(model, 2)));
}

// NOTE: Slang's float4x4(...) constructor takes ROWS (WGSL takes columns),
// so the translation goes in the 4th element of each of the first 3 rows.
public float4x4 translation_matrix(float3 t)
{
    return float4x4(1.0, 0.0, 0.0, t.x,
                    0.0, 1.0, 0.0, t.y,
                    0.0, 0.0, 1.0, t.z,
                    0.0, 0.0, 0.0, 1.0);
}

// Drops the translation of a view matrix (rotation only), e.g. for skyboxes.
public float4x4 remove_translation(float4x4 view)
{
    float4x4 m = view;
    m[0][3] = 0.0;
    m[1][3] = 0.0;
    m[2][3] = 0.0;
    m[3][3] = 1.0;
    return m;
}

public float4 local_to_world(Mesh mesh, float3 p)
{
    return mul(mesh.model, float4(p, 1.0));
}

public float4 world_to_clip(Viewport viewport, Camera camera, float4 world)
{
    return mul(viewport.projection, mul(camera.view, world));
}

// Standard path: local -> world -> clip
public float4 local_to_clip(Viewport viewport, Camera camera, Mesh mesh, float3 p)
{
    return world_to_clip(viewport, camera, local_to_world(mesh, p));
}
