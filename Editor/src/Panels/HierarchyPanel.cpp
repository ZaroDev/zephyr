#include "HierarchyPanel.h"

#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

#include <Zephyr/Input/Input.h>

#include <Zephyr/Core/Application.h>

#include <FontIcons/IconsForkAwesome.h>

#include "Zephyr/Renderer/Camera.h"

namespace Editor
{
	template<typename T, typename UIFunction>
	static void DrawComponent(const std::string& name, Zephyr::ECS::Entity entity, UIFunction uiFunction)
	{
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
		if (entity.HasComponent<T>())
		{
			auto& component = entity.GetComponent<T>();
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), treeNodeFlags, name.c_str());
			ImGui::PopStyleVar(
			);
			ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
			{
				ImGui::OpenPopup("ComponentSettings");
			}

			bool removeComponent = false;
			if (ImGui::BeginPopup("ComponentSettings"))
			{
				if (ImGui::MenuItem("Remove component"))
					removeComponent = true;

				ImGui::EndPopup();
			}

			if (open)
			{
				uiFunction(component);
				ImGui::TreePop();
			}

			if (removeComponent)
			{
				entity.RemoveComponent<T>();
			}
		}
	}


	static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f)
	{
		ImGuiIO& io = ImGui::GetIO();
		auto boldFont = io.Fonts->Fonts[0];

		ImGui::PushID(label.c_str());

		ImGui::Columns(2);
		ImGui::SetColumnWidth(0, columnWidth);
		ImGui::Text(label.c_str());
		ImGui::NextColumn();

		ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });

		float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
		ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("X", buttonSize))
			values.x = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Y", buttonSize))
			values.y = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();
		ImGui::SameLine();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
		ImGui::PushFont(boldFont);
		if (ImGui::Button("Z", buttonSize))
			values.z = resetValue;
		ImGui::PopFont();
		ImGui::PopStyleColor(3);

		ImGui::SameLine();
		ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
		ImGui::PopItemWidth();

		ImGui::PopStyleVar();

		ImGui::Columns(1);

		ImGui::PopID();
	}

	void HierarchyPanel::OnUpdate()
	{

	/*	Zephyr::Camera& cam = Zephyr::Application::Get().GetRenderer().GetMainCamera();
		cam.SetCameraTarget(glm::vec3( 0.0f, 0.0f, 0.0f ));

		if (m_SelectionContext) {
			Zephyr::ECS::TransformComponent& tcomp = m_SelectionContext.GetComponent<Zephyr::ECS::TransformComponent>();

			cam.SetCameraTarget(tcomp.Translation);
		}*/
	}

	void HierarchyPanel::OnImGui()
	{
		ImGui::Begin(m_Name.c_str(), &m_Open);

		m_Context = Zephyr::Application::Get().GetActiveScene();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
		if (ImGui::Button(ICON_FK_PLUS))
		{
			ImGui::OpenPopup("Add menu");
		}
		if (ImGui::BeginPopup("Add menu"))
		{
			if (ImGui::MenuItem("New entity"))
			{
				m_Context->CreateEntity("New entity");
			}
			ImGui::EndPopup();
		}
		ImGui::PopStyleColor();
		ImGui::SameLine();

		static ImGuiTextFilter filter;
		filter.Draw("##searchbar", 200.f);
		ImGui::Separator();

		m_Context->m_Registry.each([&](auto entityID)
			{
				Zephyr::ECS::Entity entity{ entityID, m_Context.get() };
				if (filter.PassFilter(entity.GetName().c_str()))
				{
					DrawEntityNode(entity);
				}
			});


		ImGui::End();

		ImGui::Begin("Inspector");

		if (m_SelectionContext)
		{
			DrawComponents(m_SelectionContext);
		}


		ImGui::End();
		DrawGizmos();
	}

	void HierarchyPanel::DrawGizmos()
	{
		if(m_SelectionContext && m_GizmoType != ImGuizmo::BOUNDS)
		{
			ImGuizmo::SetOrthographic(false);

			auto& deviceParams = Zephyr::Application::Get().GetDeviceManager().GetDeviceParams();

			ImGuizmo::SetRect(0, 0, deviceParams.BackBufferWidth, deviceParams.BackBufferHeight);

			/*const auto& camera = Zephyr::Application::Get().GetRenderer().GetMainCamera();
			const Zephyr::Mat4 view = camera.GetView();
			const Zephyr::Mat4 projection = camera.GetProjection();

			auto& transformCmp = m_SelectionContext.GetComponent<Zephyr::ECS::TransformComponent>();
			Zephyr::Mat4 transform = transformCmp.GetTransform();


			const bool snap = Zephyr::Input::IsKeyDown(Zephyr::Key::LeftControl);
			float snapValue = 0.5;

			if (m_GizmoType == ImGuizmo::OPERATION::ROTATE)
			{
				snapValue = 45.0f;
			}

			const float snapValues[3] = { snapValue, snapValue, snapValue };

			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
				m_GizmoType, m_GizmoMode, glm::value_ptr(transform),
				nullptr, snap ? snapValues : nullptr);

			if(ImGuizmo::IsUsing())
			{
				Zephyr::V3 translation, scale;
				Zephyr::Quaternion rotation;
				Zephyr::Math::DecomposeTransform(transform, translation, rotation, scale);

				transformCmp.Translation = translation;
				transformCmp.Rotation = rotation;
				transformCmp.Scale = scale;
			}*/
		}
	}

	void HierarchyPanel::DrawEntityNode(Zephyr::ECS::Entity entity)
	{
		auto& tag = entity.GetComponent<Zephyr::ECS::TagComponent>().Tag;

		ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_Leaf;
		flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
		ImGui::TreeNodeEx((void*)(u64)(u32)entity, flags, tag.c_str());
		if (ImGui::IsItemClicked())
		{
			m_SelectionContext = entity;
		}

		bool entityDeleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete Entity"))
				entityDeleted = true;

			ImGui::EndPopup();
		}

		if (entityDeleted)
		{
			m_Context->DestroyEntity(entity);
			if (m_SelectionContext == entity)
				m_SelectionContext = {};
		}
		ImGui::TreePop();
	}

	void HierarchyPanel::DrawComponents(Zephyr::ECS::Entity entity)
	{
		if (ImGui::RadioButton("Translate", m_GizmoType == ImGuizmo::TRANSLATE))
		{
			m_GizmoType = ImGuizmo::TRANSLATE;
		}
		ImGui::SameLine();

		if (ImGui::RadioButton("Rotate", m_GizmoType == ImGuizmo::ROTATE))
		{
			m_GizmoType = ImGuizmo::ROTATE;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", m_GizmoType == ImGuizmo::SCALE))
		{
			m_GizmoType = ImGuizmo::SCALE;
		}

		if (ImGui::RadioButton("World", m_GizmoMode == ImGuizmo::WORLD))
		{
			m_GizmoMode = ImGuizmo::WORLD;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("Local", m_GizmoMode == ImGuizmo::LOCAL))
		{
			m_GizmoMode = ImGuizmo::LOCAL;
		}

		if (entity.HasComponent<Zephyr::ECS::TagComponent>())
		{
			auto& tag = entity.GetComponent<Zephyr::ECS::TagComponent>().Tag;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
			if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
			{
				tag = std::string(buffer);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Add Component"))
		{
			ImGui::OpenPopup("AddComponent");
		}

		if (ImGui::BeginPopup("AddComponent"))
		{
			DisplayAddComponentEntry<Zephyr::ECS::MeshComponent>("Mesh");
			DisplayAddComponentEntry<Zephyr::ECS::LightComponent>("Light");

			ImGui::EndPopup();
		}

		DrawComponent<Zephyr::ECS::TransformComponent>("Transform", entity, [](auto& component)
			{
				DrawVec3Control("Translation", component.Translation);
				glm::vec3 rotation = glm::degrees(glm::eulerAngles(component.Rotation));
				DrawVec3Control("Rotation", rotation);
				component.Rotation = Zephyr::Quaternion(glm::radians(rotation));
				DrawVec3Control("Scale", component.Scale, 1.0f);
			});

		DrawComponent<Zephyr::ECS::MeshComponent>("Transform", entity, [](auto& component)
			{
				ImGui::InputInt("Mesh id", reinterpret_cast<int*>(&component.MeshId));
				ImGui::InputInt("Material id", reinterpret_cast<int*>(&component.MaterialId));
			});

		DrawComponent<Zephyr::ECS::LightComponent>("Light", entity, [](auto& component)
			{
				const char* types[] =
				{
					"Directional",
					"Point light"
				};
				static const char* currentItem = types[0];
				static Zephyr::ECS::LightComponent::Type type = component.LightType;
				if (ImGui::BeginCombo("Type", currentItem))
				{
					for (int i = 0; i < 2; i++)
					{
						bool isSelected = (currentItem == types[i]);
						if (ImGui::Selectable(types[i], isSelected))
						{
							currentItem = types[i];
							component.LightType = static_cast<Zephyr::ECS::LightComponent::Type>(i);
						}
						if (isSelected)
							ImGui::SetItemDefaultFocus();
					}

					ImGui::EndCombo();
				}
				ImGui::	ColorEdit3("Color", glm::value_ptr(component.Color));
				DrawVec3Control("Direction", component.Direction, 1.0f);
			});
	}

	template<typename T>
	void HierarchyPanel::DisplayAddComponentEntry(const std::string& entryName) {
		if (!m_SelectionContext.HasComponent<T>())
		{
			if (ImGui::MenuItem(entryName.c_str()))
			{
				m_SelectionContext.AddComponent<T>();
				ImGui::CloseCurrentPopup();
			}
		}
	}

}
