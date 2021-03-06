# Configuration files
Configuration files in Kokko are JSON formatted.

## Shaders
Shader configuration files define the vertex and fragment shader files, as well
as the shader uniforms whose values are defined by a material. It can also
optionally define the rendering type. Allowed values for renderType are opaque,
alphaTest, transparent, with the default being opaque. The allowed values for
materialUniforms.type are tex2d, mat4x4, vec4, vec3, vec2, float, int.

```
{
  "vertexShaderFile": "vert.glsl",
  "fragmentShaderFile": "frag.glsl",
  "renderType": "opaque",
  "materialUniforms":
  [
    { "name": "diffuse_map", "type": "tex2d" },
    { "name": "color_tint", "type": "vec3" }
  ]
}
```

## Materials
Material configuration files define the shader and values for the shader's
material uniform variables. Textures are defined by the file path. Vectors and
matrices are defined by arrays of numbers. Floats and integers are single
numbers.

```
{
  "shader": "diffuse.shader.json",
  "variables":
  [
    {
      "name": "diffuse_map",
      "value": "diffuse.texture.json"
    },
    {
      "name": "color_tint",
      "value": [ 1.0, 0.8, 0.6 ]
    }
  ]
}
```

## Textures
Texture configuration files define the image file or files that comprise the
texture as well as the type of the texture. Allowed values for type are tex2d
and texCube. Value for texture is a string defining the image file for 2D
texture, or a array of string defining the image files for a cube texture.

```
{
  "type": "tex2d",
  "texture": "diffuse.glraw"
}
```

```
{
  "type": "texCube",
  "texture":
  [
    "cube_x_positive.glraw",
    "cube_x_negative.glraw",
    "cube_y_positive.glraw",
    "cube_y_negative.glraw",
    "cube_z_positive.glraw",
    "cube_z_negative.glraw"
  ]
}
```
