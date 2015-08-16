#pragma once

#include "Vec3.h"
#include "Mat3x3.h"
#include "Mat4x4.h"

#include "Matrix.h"

class Transform
{
public:
	Vec3f position;
	Mat3x3f rotation;
	Vec3f scale = Vec3f(1.0f, 1.0f, 1.0f);
	
	inline Mat4x4f GetTransformMatrix() const
	{
		return Matrix::Translate(position) * Mat4x4f(rotation) * Matrix::Scale(scale);
	}
};