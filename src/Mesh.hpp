#pragma once

#include "ObjectId.hpp"
#include "BoundingBox.hpp"
#include "VertexFormat.hpp"
#include "BufferRef.hpp"

struct Mesh
{
public:
	enum class PrimitiveMode
	{
		Points,
		LineStrip,
		LineLoop,
		Lines,
		TriangleStrip,
		TriangleFan,
		Triangles
	};

private:
	enum BufferType { VertexBuffer, IndexBuffer };

	static const unsigned int primitiveModeValues[7];

	void CreateBuffers(void* vertexBuffer, unsigned int vertexBufferSize,
					   void* indexBuffer, unsigned int indexBufferSize);

public:
	//unsigned int id;

	unsigned int vertexArrayObject;
	unsigned int bufferObjects[2];

	int indexCount;
	unsigned int indexElementType;
	unsigned int primitiveMode;

	BoundingBox bounds;

	Mesh();
	Mesh(const Mesh& other) = delete;
	~Mesh();

	Mesh& operator=(Mesh&& other);

	bool HasVertexArrayObject() const { return vertexArrayObject != 0; }

	void DeleteBuffers();

	void Upload_3f(BufferRef<Vertex3f> vertices, BufferRef<unsigned short> indices);
	void Upload_3f2f(BufferRef<Vertex3f2f> vertices, BufferRef<unsigned short> indices);
	void Upload_3f3f(BufferRef<Vertex3f3f> vertices, BufferRef<unsigned short> indices);
	void Upload_3f3f2f(BufferRef<Vertex3f3f2f> vertices, BufferRef<unsigned short> indices);
	void Upload_3f3f3f(BufferRef<Vertex3f3f3f> vertices, BufferRef<unsigned short> indices);

	void SetPrimitiveMode(PrimitiveMode mode)
	{
		this->primitiveMode = primitiveModeValues[static_cast<unsigned int>(mode)];
	}

	bool LoadFromBuffer(BufferRef<unsigned char> buffer);
};
