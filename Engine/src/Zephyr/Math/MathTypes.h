/*
MIT License

Copyright (c) 2023 Víctor Falcón Zaro

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <Zephyr/Core/BasicTypes.h>
#ifdef PLATFORM_WINDOWS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_XYZW_ONLY
#endif
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Zephyr
{
	namespace Math
	{
		constexpr f32 c_Pi = 3.14159265359f; /**< Pi constant value */
		constexpr f32 c_Epsilon = 1e-5f;  /**< Epsilon constant value */
	}

	//! 2D Vector; 32 bit floating point components
	using V2 = glm::vec2;

	//! 2D Vector; 32 bit integer components 
	using Iv2 = glm::ivec2;

	//! 3D Vector; 32 bit floating point components
	using V3 = glm::vec3;

	//! 3D Vector; 32 bit integer components 
	using Iv3 = glm::ivec3;

	//! 4D Vector; 32 bit floating point components
	using V4 = glm::vec4;

	//! 4D Vector; 32 bit integer components 
	using Iv4 = glm::ivec4;

	//! 2D Vector; 32 bit unsigned integer components
	using U32V2 = glm::u32vec2;

	//! 3D Vector; 32 bit unsigned integer components
	using U32V3 = glm::u32vec3;

	//! 4D Vector; 32 bit unsigned integer components
	using U32V4 = glm::u32vec4;

	//! 2D Vector; 32 bit signed integer components
	using S32V2 = glm::i32vec2;

	//! 3D Vector; 32 bit signed integer components
	using S32V3 = glm::i32vec3;

	//! 4D Vector; 32 bit signed integer components
	using S32V4 = glm::i32vec4;

	//! 3x3 Matrix: 32 bit floating point components
	using Mat3 = glm::mat3x3;

	// 4x4 Matrix (assumes right-handed coordinates)
	using Mat4 = glm::mat4x4;

	//! Quaternion structure
	typedef glm::quat Quaternion;

	//! Color structure
	using Color = glm::vec4;

	using Color3 = V3;
	using Color4 = V4;
}