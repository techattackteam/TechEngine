#version 460 core
#extension GL_NV_uniform_buffer_std430_layout: enable
#extension GL_ARB_bindless_texture: enable

#define VOXEL_GRID_SIZE_X 160
#define VOXEL_GRID_SIZE_Y 90
#define VOXEL_GRID_SIZE_Z 128
#define BLUE_NOISE_TEXTURE_SIZE 128

// ------------------------------------------------------------------

float exp_01_to_linear_01_depth(float z, float n, float f)
{
    float z_buffer_params_y = f / n;
    float z_buffer_params_x = 1.0f - z_buffer_params_y;

    return 1.0f / (z_buffer_params_x * z + z_buffer_params_y);
}

// ------------------------------------------------------------------

float linear_01_to_exp_01_depth(float z, float n, float f)
{
    float z_buffer_params_y = f / n;
    float z_buffer_params_x = 1.0f - z_buffer_params_y;

    return (1.0f / z - z_buffer_params_y) / z_buffer_params_x;
}

// ------------------------------------------------------------------

vec3 world_to_ndc(vec3 world_pos, mat4 vp)
{
    vec4 p = vp * vec4(world_pos, 1.0f);

    if (p.w > 0.0f)
    {
        p.x /= p.w;
        p.y /= p.w;
        p.z /= p.w;
    }

    return p.xyz;
}

// ------------------------------------------------------------------

vec3 ndc_to_uv(vec3 ndc, float n, float f, float depth_power)
{
    vec3 uv;

    uv.x = ndc.x * 0.5f + 0.5f;
    uv.y = ndc.y * 0.5f + 0.5f;
    uv.z = exp_01_to_linear_01_depth(ndc.z * 0.5f + 0.5f, n, f);

    // Exponential View-Z
    vec2 params = vec2(float(VOXEL_GRID_SIZE_Z) / log2(f / n), -(float(VOXEL_GRID_SIZE_Z) * log2(n) / log2(f / n)));

    float view_z = uv.z * f;
    uv.z = (max(log2(view_z) * params.x + params.y, 0.0f)) / VOXEL_GRID_SIZE_Z;

    return uv;
}

// ------------------------------------------------------------------

vec3 world_to_uv(vec3 world_pos, float n, float f, float depth_power, mat4 vp)
{
    vec3 ndc = world_to_ndc(world_pos, vp);
    return ndc_to_uv(ndc, n, f, depth_power);
}

// ------------------------------------------------------------------

vec3 uv_to_ndc(vec3 uv, float n, float f, float depth_power)
{
    vec3 ndc;

    ndc.x = 2.0f * uv.x - 1.0f;
    ndc.y = 2.0f * uv.y - 1.0f;
    ndc.z = 2.0f * linear_01_to_exp_01_depth(uv.z, n, f) - 1.0f;

    return ndc;
}

// ------------------------------------------------------------------

vec3 ndc_to_world(vec3 ndc, mat4 inv_vp)
{
    vec4 p = inv_vp * vec4(ndc, 1.0f);

    p.x /= p.w;
    p.y /= p.w;
    p.z /= p.w;

    return p.xyz;
}

// ------------------------------------------------------------------

vec3 id_to_uv(ivec3 id, float n, float f)
{
    // Exponential View-Z
    float view_z = n * pow(f / n, (float(id.z) + 0.5f) / float(VOXEL_GRID_SIZE_Z));

    return vec3((float(id.x) + 0.5f) / float(VOXEL_GRID_SIZE_X),
    (float(id.y) + 0.5f) / float(VOXEL_GRID_SIZE_Y),
    view_z / f);
}

// ------------------------------------------------------------------

vec3 id_to_uv_with_jitter(ivec3 id, float n, float f, float jitter)
{
    // Exponential View-Z
    float view_z = n * pow(f / n, (float(id.z) + 0.5f + jitter) / float(VOXEL_GRID_SIZE_Z));

    return vec3((float(id.x) + 0.5f) / float(VOXEL_GRID_SIZE_X),
    (float(id.y) + 0.5f) / float(VOXEL_GRID_SIZE_Y),
    view_z / f);
}

// ------------------------------------------------------------------

vec3 id_to_world(ivec3 id, float n, float f, float depth_power, mat4 inv_vp)
{
    vec3 uv = id_to_uv(id, n, f);
    vec3 ndc = uv_to_ndc(uv, n, f, depth_power);
    return ndc_to_world(ndc, inv_vp);
}

// ------------------------------------------------------------------

vec3 id_to_world_with_jitter(ivec3 id, float jitter, float n, float f, float depth_power, mat4 inv_vp)
{
    vec3 uv = id_to_uv_with_jitter(id, n, f, jitter);
    vec3 ndc = uv_to_ndc(uv, n, f, depth_power);
    return ndc_to_world(ndc, inv_vp);
}

// ------------------------------------------------------------------

// https://gist.github.com/Fewes/59d2c831672040452aa77da6eaab2234
vec4 textureTricubic(sampler3D tex, vec3 coord)
{
    // Shift the coordinate from [0,1] to [-0.5, texture_size-0.5]
    vec3 texture_size = vec3(textureSize(tex, 0));
    vec3 coord_grid = coord * texture_size - 0.5;
    vec3 index = floor(coord_grid);
    vec3 fraction = coord_grid - index;
    vec3 one_frac = 1.0 - fraction;

    vec3 w0 = 1.0 / 6.0 * one_frac * one_frac * one_frac;
    vec3 w1 = 2.0 / 3.0 - 0.5 * fraction * fraction * (2.0 - fraction);
    vec3 w2 = 2.0 / 3.0 - 0.5 * one_frac * one_frac * (2.0 - one_frac);
    vec3 w3 = 1.0 / 6.0 * fraction * fraction * fraction;

    vec3 g0 = w0 + w1;
    vec3 g1 = w2 + w3;
    vec3 mult = 1.0 / texture_size;
    vec3 h0 = mult * ((w1 / g0) - 0.5 + index); //h0 = w1/g0 - 1, move from [-0.5, texture_size-0.5] to [0,1]
    vec3 h1 = mult * ((w3 / g1) + 1.5 + index); //h1 = w3/g1 + 1, move from [-0.5, texture_size-0.5] to [0,1]

    // Fetch the eight linear interpolations
    // Weighting and fetching is interleaved for performance and stability reasons
    vec4 tex000 = texture(tex, h0, 0.0f);
    vec4 tex100 = texture(tex, vec3(h1.x, h0.y, h0.z), 0.0f);
    tex000 = mix(tex100, tex000, g0.x); // Weigh along the x-direction

    vec4 tex010 = texture(tex, vec3(h0.x, h1.y, h0.z), 0.0f);
    vec4 tex110 = texture(tex, vec3(h1.x, h1.y, h0.z), 0.0f);
    tex010 = mix(tex110, tex010, g0.x); // Weigh along the x-direction
    tex000 = mix(tex010, tex000, g0.y); // Weigh along the y-direction

    vec4 tex001 = texture(tex, vec3(h0.x, h0.y, h1.z), 0.0f);
    vec4 tex101 = texture(tex, vec3(h1.x, h0.y, h1.z), 0.0f);
    tex001 = mix(tex101, tex001, g0.x); // Weigh along the x-direction

    vec4 tex011 = texture(tex, vec3(h0.x, h1.y, h1.z), 0.0f);
    vec4 tex111 = texture(tex, vec3(h1), 0.0f);
    tex011 = mix(tex111, tex011, g0.x); // Weigh along the x-direction
    tex001 = mix(tex011, tex001, g0.y); // Weigh along the y-direction

    return mix(tex001, tex000, g0.z); // Weigh along the z-direction
}

// ------------------------------------------------------------------


// ------------------------------------------------------------------
// DEFINES ----------------------------------------------------------
// ------------------------------------------------------------------

#define LOCAL_SIZE_X 8
#define LOCAL_SIZE_Y 8
#define LOCAL_SIZE_Z 1
#define M_PI 3.14159265359
#define EPSILON 0.0001f

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = LOCAL_SIZE_X, local_size_y = LOCAL_SIZE_Y, local_size_z = LOCAL_SIZE_Z) in;

// ------------------------------------------------------------------
// OUTPUT -----------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba16f) uniform writeonly image3D i_VoxelGrid;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

/**
layout (std140, binding = 0) uniform Uniforms
{
    mat4 view;
    mat4 projection;
    mat4 view_proj;
    mat4 prev_view_proj;
    mat4 light_view_proj;
    mat4 inv_view_proj;
    vec4 light_direction;
    vec4 light_color;
    vec4 camera_position;
    vec4 bias_near_far_pow;
    vec4 aniso_density_scattering_absorption;
    vec4 time;
    ivec4 width_height;
};
*/

layout (std430, binding = 0) uniform FroxelParams {
    mat4 viewProjectionInverse;
    mat4 viewMatrix;

    uvec3 froxelDimensions;
    float froxelNearPlane;

    float froxelFarPlane;
    float depthDistributionScale;
    uint useExponentialDepth;
    float rcpFroxelDimX;

    float rcpFroxelDimY;
    float rcpFroxelDimZ;
    vec2 padding_0;

    vec3 cameraPosition;
    float padding_1;
};

layout (std430, binding = 1) uniform VolumetricSettings {
    vec3 scatteringCoefficient;
    float density;

    vec3 absorptionCoefficient;
    float anisotropy;

    vec3 emissiveCoefficient;
    bool enabled;

    float blendingFactor;
    vec3 pad2;
};

struct Light {
    vec3 position;
    int type;

    vec3 direction;
    float radius;

    vec3 color;
    float intensity;

    float innerCutoff;
    float outerCutoff;
    int castShadow;
    float pad2;

    uvec2 shadowHandle[4];

    mat4 lightSpaceMatrix[4];

    float cascadeSplits[4];
};

layout (std430, binding = 0) readonly buffer LightBuffer {
    Light lights[];
};

uniform sampler2DShadow s_ShadowMap;

uniform bool u_Accumulation = false;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float sample_shadow_map(vec2 coord, float z)
{
    float current_depth = z;

    Light light = lights[0];
    sampler2D handle = sampler2D(light.shadowHandle[0]);
    float closestDepth = texture(handle, coord.xy).r;
    float bias = 0.000f;
    closestDepth += bias;
    float shadow = current_depth > closestDepth ? 1.0 : 0.0;
    return 1.0 - shadow;
}

// ------------------------------------------------------------------

float visibility(vec3 p)
{
    Light light = lights[0];
    mat4 light_view_proj = light.lightSpaceMatrix[0];
    // Transform frag position into Light-space.
    vec4 light_space_pos = light_view_proj * vec4(p, 1.0);

    // Perspective divide
    vec3 proj_coords = light_space_pos.xyz / light_space_pos.w;

    // Transform to [0,1] range
    proj_coords = proj_coords * 0.5 + 0.5;

    if (any(greaterThan(proj_coords.xy, vec2(1.0f))) || any(lessThan(proj_coords.xy, vec2(0.0f))))
    return 1.0f;

    return sample_shadow_map(proj_coords.xy, proj_coords.z);
}

// ------------------------------------------------------------------

// Henyey-Greenstein
float phase_function(vec3 Wo, vec3 Wi, float g)
{
    float cos_theta = dot(Wo, Wi);
    float denom = 1.0f + g * g + 2.0f * g * cos_theta;
    return (1.0f / (4.0f * M_PI)) * (1.0f - g * g) / max(pow(denom, 1.5f), EPSILON);
}

// ------------------------------------------------------------------

float z_slice_thickness(int z)
{
    //return 1.0f; //linear depth
    return exp(- float(VOXEL_GRID_SIZE_Z - z - 1) / float(VOXEL_GRID_SIZE_Z));
}

// ------------------------------------------------------------------

/**float sample_blue_noise(ivec3 coord)
{
    ivec2 noise_coord = (coord.xy + ivec2(0, 1) * coord.z * BLUE_NOISE_TEXTURE_SIZE) % BLUE_NOISE_TEXTURE_SIZE;
    return texelFetch(s_BlueNoise, noise_coord, 0).r;
}*/

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    ivec3 coord = ivec3(gl_GlobalInvocationID.xyz);

    if (all(lessThan(coord, ivec3(VOXEL_GRID_SIZE_X, VOXEL_GRID_SIZE_Y, VOXEL_GRID_SIZE_Z))))
    {
        // Get jitter for the current pixel, remapped to -0.5 to +0.5 range.
        //float jitter = (sample_blue_noise(coord) - 0.5f) * 0.999f;

        // Get the world position of the current voxel.
        vec3 world_pos = id_to_world(coord, froxelNearPlane, froxelFarPlane, 1.0f, viewProjectionInverse);

        // Get the view direction from the current voxel.
        vec3 Wo = normalize(cameraPosition.xyz - world_pos);

        // Density and coefficient estimation.
        float thickness = z_slice_thickness(coord.z);

        Light light = lights[0];
        // Perform lighting.
        vec3 lighting = vec3(0.0f);

        float visibility_value = visibility(world_pos);

        if (visibility_value > EPSILON) {
            lighting += visibility_value * light.color.xyz * phase_function(Wo, -light.direction.xyz, anisotropy);
        }

        // RGB = Amount of in-scattered light, A = Density.
        vec4 color_and_density = vec4(lighting * light.intensity * density, visibility_value * density);

        // Temporal accumulation
    /**        if (u_Accumulation)
        {
            vec3 world_pos_without_jitter = id_to_world(coord, bias_near_far_pow.y, bias_near_far_pow.z, bias_near_far_pow.w, inv_view_proj);

            // Find the history UV
            vec3 history_uv = world_to_uv(world_pos_without_jitter, bias_near_far_pow.y, bias_near_far_pow.z, bias_near_far_pow.w, prev_view_proj);

            // If history UV is outside the frustum, skip history
            if (all(greaterThanEqual(history_uv, vec3(0.0f))) && all(lessThanEqual(history_uv, vec3(1.0f))))
            {
                // Fetch history sample
                vec4 history = textureLod(s_History, history_uv, 0.0f);

                color_and_density = mix(history, color_and_density, 0.05f);
            }
        }*/

        // Write out lighting.
        imageStore(i_VoxelGrid, coord, color_and_density);
    }
}

// ------------------------------------------------------------------