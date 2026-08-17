#pragma once
#include <Zephyr/Core/Base.h>
#include <Zephyr/Core/BasicTypes.h>

namespace Zephyr
{
	enum class UpdateFlags : u32
	{
		None = BIT(0),
		Update = BIT(1),
		PhysicsUpdate = BIT(2),
		RendererUpdate = BIT(3)
	};

	ENUM_CLASS_FLAG_OPERATORS(UpdateFlags);

	// Interface for engine modules
	class IModule
	{
	public:
		virtual bool Initialize() = 0;
		virtual void Shutdown() = 0;

		virtual String GetName() const = 0;
		virtual i32 GetPriority() const = 0;
		virtual UpdateFlags GetUpdateFlags() const = 0;

		// Core modules will assert upon engine initialization
		virtual bool IsCoreModule() const { return false; }

		virtual void PreUpdate(float deltaTime) {}
		virtual void Update(float deltaTime) {}
		virtual void PostUpdate(float deltaTime) {}

		virtual void PrePhysicsUpdate(float deltaTime) {}
		virtual void PhysicsUpdate(float deltaTime) {}
		virtual void PostPhysicsUpdate(float deltaTime) {}

		virtual void PreRender(float deltaTime) {}
		virtual void Render(float deltaTime) {}
		virtual void PostRenderer(float deltaTime) {}
	};
}