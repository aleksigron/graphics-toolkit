in vec2 fs_tex_coord;

out vec4 color;

uniform sampler2D glyph_tex;

layout(std140, binding = BLOCK_BINDING_OBJECT) uniform DebugTextBlock
{
	vec2 shadow_offset;
};

void main()
{
	float normal = texture(glyph_tex, fs_tex_coord).r;
	float offset = texture(glyph_tex, fs_tex_coord - shadow_offset).r;

	float alpha = clamp(normal + offset, 0.0, 1.0);

	color = vec4(normal, normal, normal, alpha);
}
