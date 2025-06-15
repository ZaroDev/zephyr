#include "pch.h"
#include "Math.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace Zephyr::Math
{
	Mat4 CreateTransform(const V3& translation, const Quaternion& rotation, const V3& scale)
	{
		const auto rot = glm::toMat4(rotation);
		return glm::translate(Mat4(1.0f), translation) * rot * glm::scale(Mat4(1.0f), scale);
	}
	void DecomposeTransform(const Mat4& m, V3& translation, Quaternion& rotation, V3& scale)
	{
		V3 view;
		V4 pers;

		glm::decompose(m, scale, rotation, translation, view, pers);
	}

	float Lerp(float a, float b, float f)
	{
		return a + f * (b - a);
	}
}
