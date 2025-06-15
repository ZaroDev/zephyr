/*
MIT License

Copyright (c) 2025 ZaroDev

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

#include <string>

#include <Zephyr/Core/UUID.h>
#include <Zephyr/Math/Math.h>

namespace Zephyr::ECS
{
	struct IDComponent
	{
		UUID ID;

		IDComponent() = default;
		IDComponent(const UUID& id) : ID(id){}
		IDComponent(const IDComponent&) = default;
	};

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct MeshComponent
	{
		u32 MeshId;
		u32 MaterialId;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(u32 mesh, u32 material)
			: MeshId(mesh), MaterialId(material) {}
	};

	struct TransformComponent
	{
		V3 Translation = { 0.0f, 0.0f, 0.0f };
		Quaternion Rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		V3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(V3 translation, Quaternion rotation, V3 scale)
			: Translation(translation), Rotation(rotation), Scale(scale) {}
		TransformComponent(const V3& translation)
			: Translation(translation) {}

		TransformComponent(const Mat4& mat)
		{
			Math::DecomposeTransform(mat, Translation, Rotation, Scale);
		}

		Mat4 GetTransform() const
		{
			return Math::CreateTransform(Translation, Rotation, Scale);
		}
	};

	struct LightComponent
	{
		enum class Type
		{
			DIRECTIONAL,
			POINT_LIGHT,
			SPOT_LIGHT
		} LightType;

		V3 Color = { 1.0f, 1.0f, 1.0f };
		V3 Direction = { 0.0f, 0.0f, 0.0f };
		float Radius;

		LightComponent() = default;
		LightComponent(const LightComponent&) = default;
		LightComponent(const Type type, const V3 color, const float radius)
			: LightType(type), Color(color), Direction(0.0), Radius(radius) {}

		LightComponent(const Type type , const V3 color, const V3 direction)
			: LightType(type),Color(color), Direction(direction){}
	};

	template<typename... Component>
	struct ComponentGroup
	{
	};

	using AllComponents =
		ComponentGroup<TransformComponent, MeshComponent, LightComponent>;

}