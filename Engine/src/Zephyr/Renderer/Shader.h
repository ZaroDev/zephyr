#pragma once
#include "GraphicsAPI.h"

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
			MAIN = 0,
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


		static Ref<Shader> Create(const std::filesystem::path& fileName);
	protected:
		std::string m_Name;
	};

	class ShaderLibrary
	{
	public:
		bool LoadEngineShaders();

		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);

		Ref<Shader> Load(const std::filesystem::path& fileName);
		Ref<Shader> Load(const std::string& name, const std::filesystem::path& fileName);


		Ref<Shader> Get(const std::string& name);

		bool Exists(const std::string& name);

		static ShaderLibrary& Get();

	private:
		std::unordered_map<std::string, Ref<Shader>> m_Library;
	};
}