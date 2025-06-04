#include <pch.h>
#include "CVarManager.h"


#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

namespace Zephyr
{
	enum class CVarType : byte
	{
		BOOL,
		INT,
		FLOAT,
		STRING
	};


	class CVarParameter
	{
	public:
		u32 ArrayIndex = 0;
		CVarType Type = CVarType::BOOL;
		CVarFlags Flags = CVarFlags::NONE;

		String Name;
		String Description;

		friend class CVarManager;
	};

	template<typename T>
	struct CVarStorage
	{
		T Initial;
		T Current;
		CVarParameter* Parameter;
	};

	template<typename T>
	struct CVarArray
	{
		CVarStorage<T>* CVars;
		u32 LastCVar = 0;
		u32 Size = 0;


		CVarArray(size size)
		{
			CVars = new CVarStorage<T>[size]();
			Size = size;
		}
		~CVarArray()
		{
			delete[] CVars;
		}
		T* GetCurrentPtr(int32_t index)
		{
			CORE_ASSERT(index < Size, "Trying to access outside of array bounds");
			return &CVars[index].Current;
		};

		T GetCurrent(u32 index)
		{
			CORE_ASSERT(index < Size, "Trying to access outside of array bounds");
			return CVars[index].Current;
		}

		void SetCurrent(const T& value, u32 index)
		{
			CORE_ASSERT(index < Size, "Trying to access outside of array bounds");
			CVars[index].Current = value;
		}

		u32 Add(const T& value, CVarParameter* parameter)
		{
			u32 index = LastCVar;
			CORE_ASSERT(index < Size, "Trying to add a value for a full array");
			CVars[index].Current = value;
			CVars[index].Initial = value;
			CVars[index].Parameter = parameter;

			parameter->ArrayIndex = index;
			LastCVar++;
			return index;
		}
		
	};

	class CVarManagerImpl : public CVarManager
	{
	public:
		template<typename T>
		CVarArray<T>* GetCVarArray();

		template<>
		CVarArray<bool>* GetCVarArray()
		{
			return &m_BoolCVars;
		}

		template<>
		CVarArray<i32>* GetCVarArray()
		{
			return &m_IntCVars;
		}

		template<>
		CVarArray<f32>* GetCVarArray()
		{
			return &m_FloatCVars;
		}

		template<>
		CVarArray<String>* GetCVarArray()
		{
			return &m_StringCVars;
		}

		template<typename T>
		std::optional<T> GetCVarCurrent(u32 nameHash)
		{
			CVarParameter* par = GetCVar(nameHash);
			if (!par)
			{
				return std::nullopt;
			}



			return std::optional<T>(GetCVarArray<T>()->GetCurrent(par->ArrayIndex));
		}

		template<typename T>
		void SetCVarCurrent(u32 hashName, const T& value)
		{
			CVarParameter* par = GetCVar(hashName);
			if (par)
			{
				GetCVarArray<T>()->SetCurrent(value, par->ArrayIndex);
			}
		}

		CVarParameter* GetCVar(StringUtils::StringHash hash) override;
		CVarParameter* CreateBoolCVar(const char* name, const char* description, bool defaultValue, bool currentValue) override;
		CVarParameter* CreateIntCVar(const char* name, const char* description, i32 defaultValue, i32 currentValue) override;
		CVarParameter* CreateFloatCVar(const char* name, const char* description, f32 defaultValue, f32 currentValue) override;
		CVarParameter* CreateStringCVar(const char* name, const char* description, String defaultValue, String currentValue) override;

		std::optional<bool> GetBoolCVar(StringUtils::StringHash hash) override;
		void SetBoolCVar(StringUtils::StringHash hash, bool value) override;

		std::optional<i32> GetIntCVar(StringUtils::StringHash hash) override;
		void SetIntCVar(StringUtils::StringHash hash, i32 value) override;

		std::optional<f32> GetFloatCVar(StringUtils::StringHash hash) override;
		void SetFloatCVar(StringUtils::StringHash hash, f32 value) override;

		std::optional<String> GetStringCVar(StringUtils::StringHash hash) override;
		void SetStringCVar(StringUtils::StringHash hash, const String& value) override;

		void DrawImGuiEditor() override;
		void EditParameter(CVarParameter* p, float textWidth);

	private:
		constexpr static u32 c_MaxBoolCVars = 1000;
		CVarArray<bool> m_BoolCVars{ c_MaxBoolCVars };

		constexpr static u32 c_MaxIntCVars = 1000;
		CVarArray<i32> m_IntCVars{ c_MaxIntCVars };

		constexpr static u32 c_MaxFloatCVars = 1000;
		CVarArray<f32> m_FloatCVars{ c_MaxFloatCVars };

		constexpr static u32 c_MaxStringCVars = 1000;
		CVarArray<String> m_StringCVars{ c_MaxStringCVars };

		CVarParameter* InitCVar(const char* name, const char* description);

		std::unordered_map<u32, CVarParameter> m_SavedCVars;
		std::vector<CVarParameter*> m_CachedEditParameters;
	};

	CVarManager* CVarManager::Get()
	{
		static CVarManagerImpl sCvarMan{};
		return &sCvarMan;
	}
	CVarParameter* CVarManagerImpl::InitCVar(const char* name, const char* description)
	{
		if (GetCVar(name))
		{
			return nullptr;
		}

		u32 nameHash = StringUtils::StringHash(name);
		m_SavedCVars[nameHash] = CVarParameter();

		CVarParameter& newParam = m_SavedCVars[nameHash];
		newParam.Name = name;
		newParam.Description = description;

		return &newParam;
	}





	// Inherited via CVarManager
	CVarParameter* CVarManagerImpl::GetCVar(StringUtils::StringHash hash)
	{
		if (m_SavedCVars.contains(hash))
		{
			return &m_SavedCVars[hash];
		}

		return nullptr;
	}
	CVarParameter* CVarManagerImpl::CreateBoolCVar(const char* name, const char* description, bool defaultValue, bool currentValue)
	{
		CVarParameter* param = InitCVar(name, description);
		if (!param)
		{
			return nullptr;
		}

		param->Type = CVarType::BOOL;
		GetCVarArray<bool>()->Add(defaultValue, param);
		return param;
	}
	CVarParameter* CVarManagerImpl::CreateIntCVar(const char* name, const char* description, i32 defaultValue, i32 currentValue)
	{
		CVarParameter* param = InitCVar(name, description);
		if (!param)
		{
			return nullptr;
		}

		param->Type = CVarType::INT;
		GetCVarArray<i32>()->Add(defaultValue, param);
		return param;
	}
	CVarParameter* CVarManagerImpl::CreateFloatCVar(const char* name, const char* description, f32 defaultValue, f32 currentValue)
	{
		CVarParameter* param = InitCVar(name, description);
		if (!param)
		{
			return nullptr;
		}

		param->Type = CVarType::FLOAT;
		GetCVarArray<f32>()->Add(defaultValue, param);
		return param;
	}
	CVarParameter* CVarManagerImpl::CreateStringCVar(const char* name, const char* description, String defaultValue, String currentValue)
	{
		CVarParameter* param = InitCVar(name, description);
		if (!param)
		{
			return nullptr;
		}

		param->Type = CVarType::STRING;
		GetCVarArray<String>()->Add(defaultValue, param);
		return param;
	}

	std::optional<bool> CVarManagerImpl::GetBoolCVar(StringUtils::StringHash hash)
	{
		return GetCVarCurrent<bool>(hash);
	}
	void CVarManagerImpl::SetBoolCVar(StringUtils::StringHash hash, bool value)
	{
		SetCVarCurrent<bool>(hash, value);
	}
	std::optional<i32> CVarManagerImpl::GetIntCVar(StringUtils::StringHash hash)
	{
		return GetCVarCurrent<i32>(hash);
	}
	void CVarManagerImpl::SetIntCVar(StringUtils::StringHash hash, i32 value)
	{
		SetCVarCurrent<i32>(hash, value);
	}
	std::optional<f32> CVarManagerImpl::GetFloatCVar(StringUtils::StringHash hash)
	{
		return GetCVarCurrent<f32>(hash);
	}
	void CVarManagerImpl::SetFloatCVar(StringUtils::StringHash hash, f32 value)
	{
		SetCVarCurrent<f32>(hash, value);
	}
	std::optional<String> CVarManagerImpl::GetStringCVar(StringUtils::StringHash hash)
	{
		return GetCVarCurrent<String>(hash);
	}
	void CVarManagerImpl::SetStringCVar(StringUtils::StringHash hash, const String& value)
	{
		SetCVarCurrent<String>(hash, value);
	}

	void CVarManagerImpl::DrawImGuiEditor()
	{
		static String searchText = "";

		ImGui::InputText("Filter", &searchText);
		static bool bShowAdvanced = false;
		ImGui::Checkbox("Advanced", &bShowAdvanced);
		ImGui::Separator();
		m_CachedEditParameters.clear();

		auto addToEditList = [&](auto parameter)
			{
				bool bHidden = ((u32)parameter->Flags & (uint32_t)CVarFlags::NO_EDIT);
				bool bIsAdvanced = ((u32)parameter->Flags & (uint32_t)CVarFlags::ADVANCED);

				if (!bHidden)
				{
					if (!(!bShowAdvanced && bIsAdvanced) && parameter->Name.find(searchText) != std::string::npos)
					{
						m_CachedEditParameters.push_back(parameter);
					};
				}
			};

		for (i32 i = 0; i < GetCVarArray<bool>()->LastCVar; i++)
		{
			addToEditList(GetCVarArray<bool>()->CVars[i].Parameter);
		}
		for (i32 i = 0; i < GetCVarArray<i32>()->LastCVar; i++)
		{
			addToEditList(GetCVarArray<i32>()->CVars[i].Parameter);
		}
		for (int i = 0; i < GetCVarArray<f32>()->LastCVar; i++)
		{
			addToEditList(GetCVarArray<f32>()->CVars[i].Parameter);
		}
		for (int i = 0; i < GetCVarArray<String>()->LastCVar; i++)
		{
			addToEditList(GetCVarArray<String>()->CVars[i].Parameter);
		}

		if (m_CachedEditParameters.size() > 10)
		{
			std::unordered_map<String, std::vector<CVarParameter*>> categorizedParams;

			//insert all the edit parameters into the hashmap by category
			for (auto p : m_CachedEditParameters)
			{
				int dotPos = -1;
				//find where the first dot is to categorize
				for (int i = 0; i < p->Name.length(); i++)
				{
					if (p->Name[i] == '.')
					{
						dotPos = i;
						break;
					}
				}
				String category = "";
				if (dotPos != -1)
				{
					category = p->Name.substr(0, dotPos);
				}

				auto it = categorizedParams.find(category);
				if (it == categorizedParams.end())
				{
					categorizedParams[category] = std::vector<CVarParameter*>();
					it = categorizedParams.find(category);
				}
				it->second.push_back(p);
			}

			for (auto [category, parameters] : categorizedParams)
			{
				//alphabetical sort
				std::sort(parameters.begin(), parameters.end(), [](CVarParameter* A, CVarParameter* B)
					{
						return A->Name < B->Name;
					});

				if (ImGui::BeginMenu(category.c_str()))
				{
					float maxTextWidth = 0;

					for (auto p : parameters)
					{
						maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(p->Name.c_str()).x);
					}
					for (auto p : parameters)
					{
						EditParameter(p, maxTextWidth);
					}

					ImGui::EndMenu();
				}
			}
		}
		else
		{
			//alphabetical sort
			std::sort(m_CachedEditParameters.begin(), m_CachedEditParameters.end(), [](CVarParameter* A, CVarParameter* B)
				{
					return A->Name < B->Name;
				});
			float maxTextWidth = 0;
			for (auto p : m_CachedEditParameters)
			{
				maxTextWidth = std::max(maxTextWidth, ImGui::CalcTextSize(p->Name.c_str()).x);
			}
			for (auto p : m_CachedEditParameters)
			{
				EditParameter(p, maxTextWidth);
			}
		}
	}
	void Label(const char* label, float textWidth)
	{
		constexpr float Slack = 50;
		constexpr float EditorWidth = 100;

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImVec2 lineStart = ImGui::GetCursorScreenPos();
		const ImGuiStyle& style = ImGui::GetStyle();
		float fullWidth = textWidth + Slack;

		ImVec2 textSize = ImGui::CalcTextSize(label);

		ImVec2 startPos = ImGui::GetCursorScreenPos();

		ImGui::Text(label);

		ImVec2 finalPos = { startPos.x + fullWidth, startPos.y };

		ImGui::SameLine();
		ImGui::SetCursorScreenPos(finalPos);

		ImGui::SetNextItemWidth(EditorWidth);
	}
	void CVarManagerImpl::EditParameter(CVarParameter* p, float textWidth)
	{
		const bool readonlyFlag = ((u32)p->Flags & (u32)CVarFlags::EDIT_READ_ONLY);
		const bool checkboxFlag = ((u32)p->Flags & (u32)CVarFlags::EDIT_CHECKBOX);
		const bool dragFlag = ((u32)p->Flags & (u32)CVarFlags::EDIT_FLOAT_DRAG);


		switch (p->Type)
		{
		case CVarType::INT:

			if (readonlyFlag)
			{
				String displayFormat = p->Name + "= %i";
				ImGui::Text(displayFormat.c_str(), GetCVarArray<i32>()->GetCurrent(p->ArrayIndex));
			}
			else
			{
				if (checkboxFlag)
				{
					bool bCheckbox = GetCVarArray<i32>()->GetCurrent(p->ArrayIndex) != 0;
					Label(p->Name.c_str(), textWidth);

					ImGui::PushID(p->Name.c_str());

					if (ImGui::Checkbox("", &bCheckbox))
					{
						GetCVarArray<i32>()->SetCurrent(bCheckbox ? 1 : 0, p->ArrayIndex);
					}
					ImGui::PopID();
				}
				else
				{
					Label(p->Name.c_str(), textWidth);
					ImGui::PushID(p->Name.c_str());
					ImGui::InputInt("", GetCVarArray<i32>()->GetCurrentPtr(p->ArrayIndex));
					ImGui::PopID();
				}
			}
			break;

		case CVarType::FLOAT:

			if (readonlyFlag)
			{
				String displayFormat = p->Name + "= %f";
				ImGui::Text(displayFormat.c_str(), GetCVarArray<f32>()->GetCurrent(p->ArrayIndex));
			}
			else
			{
				Label(p->Name.c_str(), textWidth);
				ImGui::PushID(p->Name.c_str());
				if (dragFlag)
				{
					ImGui::InputFloat("", GetCVarArray<f32>()->GetCurrentPtr(p->ArrayIndex), 0, 0, "%.3f");
				}
				else
				{
					ImGui::InputFloat("", GetCVarArray<f32>()->GetCurrentPtr(p->ArrayIndex), 0, 0, "%.3f");
				}
				ImGui::PopID();
			}
			break;

		case CVarType::STRING:

			if (readonlyFlag)
			{
				String displayFormat = p->Name + "= %s";
				ImGui::PushID(p->Name.c_str());
				ImGui::Text(displayFormat.c_str(), GetCVarArray<String>()->GetCurrent(p->ArrayIndex));

				ImGui::PopID();
			}
			else
			{
				Label(p->Name.c_str(), textWidth);
				ImGui::InputText("", GetCVarArray<String>()->GetCurrentPtr(p->ArrayIndex));

				ImGui::PopID();
			}
			break;

		default:
			break;
		}

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip(p->Description.c_str());
		}
	}

	template<typename T>
	T GetCVarCurrentByIndex(u32 index)
	{
		return dynamic_cast<CVarManagerImpl*>(CVarManagerImpl::Get())->GetCVarArray<T>()->GetCurrent(index);
	}
	template<typename T>
	void SetCVarCurrentByIndex(u32 index, const T& data)
	{
		dynamic_cast<CVarManagerImpl*>(CVarManagerImpl::Get())->GetCVarArray<T>()->SetCurrent(data, index);
	}

	AutoCVar_Bool::AutoCVar_Bool(const char* name, const char* description, bool defaultValue, CVarFlags flags)
	{
		CVarParameter* cvar = CVarManager::Get()->CreateBoolCVar(name, description, defaultValue, defaultValue);
		cvar->Flags = flags;
		m_Index = cvar->ArrayIndex;
	}
	bool AutoCVar_Bool::Get()
	{
		return GetCVarCurrentByIndex<bool>(m_Index);
	}
	void AutoCVar_Bool::Set(bool value)
	{
		SetCVarCurrentByIndex<bool>(m_Index, value);
	}

	AutoCVar_Int::AutoCVar_Int(const char* name, const char* description, i32 defaultValue, CVarFlags flags)
	{
		CVarParameter* cvar = CVarManager::Get()->CreateIntCVar(name, description, defaultValue, defaultValue);
		cvar->Flags = flags;
		m_Index = cvar->ArrayIndex;
	}
	i32 AutoCVar_Int::Get()
	{
		return GetCVarCurrentByIndex<i32>(m_Index);
	}
	void AutoCVar_Int::Set(i32 value)
	{
		SetCVarCurrentByIndex<i32>(m_Index, value);
	}

	AutoCVar_Float::AutoCVar_Float(const char* name, const char* description, f32 defaultValue, CVarFlags flags)
	{
		CVarParameter* cvar = CVarManager::Get()->CreateFloatCVar(name, description, defaultValue, defaultValue);
		cvar->Flags = flags;
		m_Index = cvar->ArrayIndex;
	}
	f32 AutoCVar_Float::Get()
	{
		return GetCVarCurrentByIndex<f32>(m_Index);
	}
	void AutoCVar_Float::Set(f32 value)
	{
		SetCVarCurrentByIndex<f32>(m_Index, value);
	}

	AutoCVar_String::AutoCVar_String(const char* name, const char* description, const char* defaultValue, CVarFlags flags)
	{
		CVarParameter* cvar = CVarManager::Get()->CreateStringCVar(name, description, defaultValue, defaultValue);
		cvar->Flags = flags;
		m_Index = cvar->ArrayIndex;
	}
	void AutoCVar_String::Set(const String& value)
	{
		SetCVarCurrentByIndex<String>(m_Index, value);
	}
	String AutoCVar_String::Get()
	{
		return GetCVarCurrentByIndex<String>(m_Index);
	}
	
}