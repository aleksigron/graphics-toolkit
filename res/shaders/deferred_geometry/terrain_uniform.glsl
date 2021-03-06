
layout(std140, binding = BLOCK_BINDING_OBJECT) uniform TerrainBlock
{
	mat4x4 MVP;
	mat4x4 MV;
    vec2 texture_scale;
    float terrain_size;
    float terrain_resolution;
    float min_height;
    float max_height;
}
uniforms;
