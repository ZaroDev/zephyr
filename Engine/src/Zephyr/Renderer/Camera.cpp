#include <pch.h>
#include "Camera.h"
#include <Zephyr/Input/Input.h>


namespace Zephyr
{
	Camera::Camera()
	{
		m_ForwardDirection = glm::vec3(0, 0, -1);
		m_Position = glm::vec3(0, 0, 5);
		RecalculateView();
	}
	Camera::Camera(float verticalFOV, float nearClip, float farClip)
		: m_VerticalFOV(verticalFOV), m_NearClip(nearClip), m_FarClip(farClip)
	{
		m_ForwardDirection = glm::vec3(0, 0, -1);
		m_Position = glm::vec3(0, 0, 5);
		RecalculateView();

		//m_Frustum = Math::Frustum(m_Projection * m_View);
	}

	bool Camera::OnUpdate(float ts)
	{
		bool moved = false;

		V2 mousePos = Input::GetMousePosition();
		V2 delta = (mousePos - m_LastMousePosition) * 0.002f;
		m_LastMousePosition = mousePos;

		if (!Input::IsMouseButtonDown(MouseButton::Right))
		{
			Input::SetCursorMode(CursorMode::Normal);
			return false;
		}

		Input::SetCursorMode(CursorMode::Locked);

		static bool orbiting = false;

		if (Input::IsKeyDown(Key::LeftAlt)) {
			if (!orbiting) {
				m_TargetDist = glm::distance(m_Position, m_TargetPos);
				orbiting = true;
			}

			moved = OrbitCameraControls(delta, ts);
		}
		else {
			orbiting = false;

			moved = NormalCameraControls(delta, ts);
		}


		if (moved)
		{
			RecalculateView();
		}


		return moved;
	}

	void Camera::OnResize(u32 width, u32 height)
	{
		if (width == m_ViewportWidth && height == m_ViewportHeight)
		{
			return;
		}

		m_ViewportWidth = width;
		m_ViewportHeight = height;

		const float ar = static_cast<float>(m_ViewportWidth / m_ViewportHeight);

		if (ar >= 1.0)
		{
			m_Parameters.Left = -ar * m_ViewportWidth;
			m_Parameters.Right = ar * m_ViewportWidth;
			m_Parameters.Top = m_ViewportWidth;
			m_Parameters.Bottom = -m_ViewportWidth;
		}
		else
		{
			m_Parameters.Left = -m_ViewportWidth;
			m_Parameters.Right = m_ViewportWidth;
			m_Parameters.Top = m_ViewportWidth / ar;
			m_Parameters.Bottom = -m_ViewportWidth / ar;
		}

		RecalculateProjection();
	}

	float Camera::GetRotationSpeed()
	{
		return 0.3f;
	}

	/*bool Camera::IsInsideFrustum(const Math::AABB& boundingBox) const
	{
		if (!FrustumCulling)
		{
			return true;
		}

		return m_Frustum.IsBoxVisible(boundingBox.GetMin(), boundingBox.GetMax());
	}*/

	bool Camera::NormalCameraControls(const V2& delta, float ts)
	{
		bool moved = false;

		constexpr V3 upDirection(0.0f, 1.0f, 0.0f);
		V3 rightDirection = glm::cross(m_ForwardDirection, upDirection);

		float speed = 0.005f;

		// Movement
		if (Input::IsKeyDown(KeyCode::W))
		{
			m_Position += m_ForwardDirection * speed * ts;
			moved = true;
		}
		else if (Input::IsKeyDown(KeyCode::S))
		{
			m_Position -= m_ForwardDirection * speed * ts;
			moved = true;
		}
		if (Input::IsKeyDown(KeyCode::A))
		{
			m_Position -= rightDirection * speed * ts;
			moved = true;
		}
		else if (Input::IsKeyDown(KeyCode::D))
		{
			m_Position += rightDirection * speed * ts;
			moved = true;
		}
		if (Input::IsKeyDown(KeyCode::Q))
		{
			m_Position -= upDirection * speed * ts;
			moved = true;
		}
		else if (Input::IsKeyDown(KeyCode::E))
		{
			m_Position += upDirection * speed * ts;
			moved = true;
		}

		// Rotation
		if (delta.x != 0.0f || delta.y != 0.0f)
		{
			float pitchDelta = delta.y * GetRotationSpeed();
			float yawDelta = delta.x * GetRotationSpeed();

			Quaternion q = glm::normalize(glm::cross(glm::angleAxis(-pitchDelta, rightDirection),
				glm::angleAxis(-yawDelta, V3(0.f, 1.0f, 0.0f))));
			m_ForwardDirection = glm::rotate(q, m_ForwardDirection);

			moved = true;
		}

		return moved;
	}

	bool Camera::OrbitCameraControls(const V2& delta, float ts)
	{
		static float angleX = 0.0f;

		bool moved = false;

		// Rotation
		if (delta.x != 0.0f || delta.y != 0.0f)
		{
			angleX += -delta.x * GetRotationSpeed();

			// Horizontal orbit
			m_Position.x = m_TargetPos.x + glm::sin(angleX) * m_TargetDist;
			m_Position.z = m_TargetPos.z + glm::cos(angleX) * m_TargetDist;

			m_ForwardDirection = glm::normalize(m_TargetPos - m_Position);

			moved = true;
		}

		return moved;
	}

	void Camera::RecalculateProjection()
	{
		m_Projection = glm::perspectiveFov(glm::radians(m_VerticalFOV), (float)m_ViewportWidth, (float)m_ViewportHeight, m_NearClip, m_FarClip);
		m_InverseProjection = glm::inverse(m_Projection);
		//m_Frustum = Math::Frustum(m_Projection * m_View);
	}

	void Camera::RecalculateView()
	{
		m_View = glm::lookAt(m_Position, m_Position + m_ForwardDirection, glm::vec3(0, 1, 0));
		m_InverseView = glm::inverse(m_View);
		//m_Frustum = Math::Frustum(m_Projection * m_View);
	}
}