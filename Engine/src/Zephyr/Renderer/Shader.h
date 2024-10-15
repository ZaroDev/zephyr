#pragma once
#include "GraphicsAPI.h"
#include <unordered_map>

namespace Zephyr
{
	struct ShaderType
	{
		enum
		{
			VERTEX = 0,
			FRAGMENT,

			MAX
		};
	};
	struct EngineShader 
	{
		enum
		{
			FULLSCREENQUAD = 0,
			GEOMETRY,
			MAX
		};
	};

	class Shader
	{
	public:
		virtual ~Shader() = default;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		const std::string& Name() const { return m_Name; }


		static Ref<Shader> Create(const Path& fileName);
	protected:
		std::string m_Name;
	};

	class ShaderLibrary
	{
	public:
		bool LoadEngineShaders();

		void Add(const String& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);

		Ref<Shader> Load(const Path& fileName);
		Ref<Shader> Load(const std::string& name, const Path& fileName);


		Ref<Shader> Get(const String& name);

		bool Exists(const String& name);


	private:
		std::unordered_map<std::string, Ref<Shader>> m_Library;
	};
}