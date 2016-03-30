#include "Texture.hpp"

#define GLFW_INCLUDE_GLCOREARB
#include "glfw/glfw3.h"

#include "ImageData.hpp"

void Texture::Upload(const ImageData& image)
{
	textureSize = image.imageSize;

	glGenTextures(1, &driverId);
	glBindTexture(GL_TEXTURE_2D, driverId);

	if (image.compressed)
	{
		// Upload compressed texture data
		glCompressedTexImage2D(GL_TEXTURE_2D, 0, image.pixelFormat,
							   image.imageSize.x, image.imageSize.y, 0,
							   image.compressedSize, image.imageData);
	}
	else
	{
		// Upload uncompressed texture data
		glTexImage2D(GL_TEXTURE_2D, 0, image.pixelFormat,
					 image.imageSize.x, image.imageSize.y, 0,
					 image.pixelFormat, image.componentDataType, image.imageData);
	}

	// Set filter
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Unbind texture
	glBindTexture(GL_TEXTURE_2D, 0);
}
