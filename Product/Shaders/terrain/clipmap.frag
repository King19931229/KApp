layout(location = 0) in vec2 inUV;
layout(location = 1) in float inLerp;
layout(location = 2) in float inHeight;

layout(location = 0) out vec4 outColor;

#define PALETTE_ARRAY_SIZE 3
const vec3 heightColorPalette[PALETTE_ARRAY_SIZE] = { vec3(0,0,1),  vec3(0,1,0),  vec3(1,0,0) };

void main()
{
    float height = clamp(inHeight, 0, 1);
    float idx = height * (PALETTE_ARRAY_SIZE - 1);
    int idx_f = int(floor(idx));
    int idx_c = int(ceil(idx));
    float factor = idx - idx_f;
    outColor = vec4(mix(heightColorPalette[idx_f], heightColorPalette[idx_c], factor), 1.0);
    // outColor = vec4(1, 0.0, 0.0, 1.0);
}