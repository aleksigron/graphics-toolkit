import bpy
from array import array
from struct import pack

def process_mesh(original, triangulated):
    import bmesh
    bm = bmesh.new()
    bm.from_mesh(original)
    bmesh.ops.triangulate(bm, faces = bm.faces)
    bm.to_mesh(triangulated)
    bm.free()

def write(context, filepath, options):
    
    scene = context.scene
    obj = context.active_object
    
    if obj.type != 'MESH': return False
    
    # Create a new mesh we can edit
    mesh_data = bpy.data.meshes.new(obj.data.name)
    
    # Triangulate mesh copy
    process_mesh(obj.data, mesh_data)
    
    # Get vertex color and texture coordinate layer counts
    vert_color_count = len(mesh_data.vertex_colors)
    tex_coord_count = len(mesh_data.uv_layers)
    
    # Decide which vertex data components to save
    save_normal = options['save_normal']
    save_vert_color = options['save_vert_color'] and vert_color_count > 0
    save_tex_coord = options['save_tex_coord'] and tex_coord_count > 0
    
    # Vertex data components
    vert_position_comp = 1 << 0 # 1 position
    vert_normal_comp = 1 << 1 # [0,1] normals
    vert_color_comp = 1 << 2 # [0,1] colors
    vert_texcoord_comp = 1 << 3 # [0,1] texture coords
    
    # Combine vertex data components to one integer
    vert_comps = vert_position_comp
    if save_normal: vert_comps = vert_comps | vert_normal_comp
    if save_vert_color: vert_comps = vert_comps | vert_color_comp
    if save_tex_coord: vert_comps = vert_comps | vert_texcoord_comp
    
    vert_count = len(mesh_data.vertices)
    
    #epsilon = 1 / (2 ** 20)
    
    vert_col_data = array('f')
    
    if save_vert_color:
        # Create an empty array of required size
        for i in range(0, vert_count * 3):
            vert_col_data.append(0.0)
        
        # Shorter name for vertex color data
        mesh_vert_col = mesh_data.vertex_colors.active
        
        # Get the vertex color for each corner of each polygon
        # Write colors to the corresponding vertex index in the array
        for i in range(0, len(mesh_data.loops)):
            v_index = mesh_data.loops[i].vertex_index
            v_color = mesh_vert_col.data[i].color
            vert_col_data[v_index * 3 + 0] = v_color[0]
            vert_col_data[v_index * 3 + 1] = v_color[1]
            vert_col_data[v_index * 3 + 2] = v_color[2]
    
    # Shorter name for mesh vertices
    verts = mesh_data.vertices
    
    # 'f' for 4-byte floating point
    vertex_data = array('f')
    
    # Put vertex data in the array
    
    # Just vertex position
    if vert_comps == vert_position_comp:
        for v in verts:
            vertex_data.extend([v.co.x, v.co.z, -v.co.y])
            
    # Vertex position and normal
    elif vert_comps == (vert_position_comp | vert_normal_comp):
        for v in verts:
            vertex_data.extend([v.co.x, v.co.z, -v.co.y])
            vertex_data.extend([v.normal.x, v.normal.z, -v.normal.y])
    
    # Vertex position and color
    elif vert_comps == (vert_position_comp | vert_color_comp):
        for i in range(0, vert_count):
            vertex_data.append(verts[i].co.x)
            vertex_data.append(verts[i].co.z)
            vertex_data.append(-verts[i].co.y)
            vertex_data.append(vert_col_data[i * 3 + 0])
            vertex_data.append(vert_col_data[i * 3 + 1])
            vertex_data.append(vert_col_data[i * 3 + 2])
    
    # Vertex position, normal and color
    elif vert_comps == (vert_position_comp | vert_normal_comp | vert_color_comp):
        for i in range(0, vert_count):
            vertex_data.append(verts[i].co.x)
            vertex_data.append(verts[i].co.z)
            vertex_data.append(-verts[i].co.y)
            vertex_data.append(verts[i].normal.x)
            vertex_data.append(verts[i].normal.z)
            vertex_data.append(-verts[i].normal.y)
            vertex_data.append(vert_col_data[i * 3 + 0])
            vertex_data.append(vert_col_data[i * 3 + 1])
            vertex_data.append(vert_col_data[i * 3 + 2])
    
    # 'H': unsigned 2-byte int, 'I': unsigned 4-byte int
    if vert_count <= (1 << 16):
        index_data = array('H')
    else:
        index_data = array('I')
    
    # Get index data
    for p in mesh_data.polygons:
        index_data.extend([p.vertices[0], p.vertices[1], p.vertices[2]])
    
    # Prepare data for the header
    index_count = len(index_data)
    
    # '=': native byte-order, standard packing
    # 'I': unsigned 4-byte integer
    header = pack('=III', vert_comps, vert_count, index_count)
    
    # Remove the mesh we created earlier
    bpy.data.meshes.remove(mesh_data)

    # Open output file
    with open(filepath, 'wb') as outfile:

        # Start with file type magic number
        outfile.write(b'\x10\x10\x19\x91');
        
        # Write header data
        outfile.write(header)
        
        # Write vertex data
        vertex_data.tofile(outfile)
        
        # Write index data
        index_data.tofile(outfile)
        
    return True
