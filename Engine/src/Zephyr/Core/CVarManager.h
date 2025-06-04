#pragma once
#include <Zephyr/Utils/StringUtils.h>

namespace Zephyr
{
	class CVarParameter;

	enum class CVarFlags : u32
	{
		NONE = 0,
		NO_EDIT = BIT(1),
		EDIT_READ_ONLY = BIT(2),
		ADVANCED = BIT(3),

		EDIT_CHECKBOX = BIT(8),
		EDIT_FLOAT_DRAG = BIT(9),
	};

	class CVarManager
	{
	public:
		NODISCARD static CVarManager* Get();

		virtual CVarParameter* GetCVar(StringUtils::StringHash hash) = 0;


		virtual CVarParameter* CreateBoolCVar(const char* name, const char* description, bool defaultValue, bool currentValue) = 0;
		virtual CVarParameter* CreateIntCVar(const char* name, const char* description, i32 defaultValue, i32 currentValue) = 0;
		virtual CVarParameter* CreateFloatCVar(const char* name, const char* description, f32 defaultValue, f32 currentValue) = 0;
		virtual CVarParameter* CreateStringCVar(const char* name, const char* description, String defaultValue, String currentValue) = 0;


		virtual std::optional<bool> GetBoolCVar(StringUtils::StringHash hash) = 0;
		virtual void SetBoolCVar(StringUtils::StringHash hash, bool value) = 0;

		virtual std::optional<i32> GetIntCVar(StringUtils::StringHash hash) = 0;
		virtual void SetIntCVar(StringUtils::StringHash hash, i32 value) = 0;

		virtual std::optional<f32> GetFloatCVar(StringUtils::StringHash hash) = 0;
		virtual void SetFloatCVar(StringUtils::StringHash hash, f32 value) = 0;

		virtual std::optional<String> GetStringCVar(StringUtils::StringHash hash) = 0;
		virtual void SetStringCVar(StringUtils::StringHash hash, const String& value) = 0;

		virtual void DrawImGuiEditor() = 0;
	};

	template<typename T>
	struct AutoCVar
	{
	protected:
		u32 m_Index;
		using CVarType = T;
	};

	struct AutoCVar_Bool : AutoCVar<bool>
	{
		AutoCVar_Bool(const char* name, const char* description, bool defaultValue, CVarFlags flags = CVarFlags::NONE);

		bool Get();
		void Set(bool value);
	};

	struct AutoCVar_Int : AutoCVar<i32>
	{
		AutoCVar_Int(const char* name, const char* description, i32 defaultValue, CVarFlags flags = CVarFlags::NONE);
		i32 Get();
		void Set(i32 value);
	};

	struct AutoCVar_Float : AutoCVar<f32>
	{
		AutoCVar_Float(const char* name, const char* description, f32 defaultValue, CVarFlags flags = CVarFlags::NONE);
		f32 Get();
		void Set(f32 value);
	};


	struct AutoCVar_String : AutoCVar<String>
	{
		AutoCVar_String(const char* name, const char* description, const char* defaultValue, CVarFlags flags = CVarFlags::NONE);

		String Get();
		void Set(const String& value);
	};
}