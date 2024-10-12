#pragma once
namespace Zephyr
{
	struct CameraParameters
	{
		float Top;
		float Bottom;
		float Left;
		float Right;
	};

	class Camera
	{
	public:
		Camera();
		Camera(float verticalFOV, float nearClip, float farClip);

		bool OnUpdate(float ts);
		void OnResize(u32 width, u32 height);

		NODISCARD const Mat4& GetProjection() const { return m_Projection; }
		NODISCARD const Mat4& GetInverseProjection() const { return m_InverseProjection; }
		NODISCARD const Mat4& GetView() const { return m_View; }
		NODISCARD const Mat4& GetInverseView() const { return m_InverseView; }

		NODISCARD const V3& GetPosition() const { return m_Position; }
		NODISCARD const V3& GetDirection() const { return m_ForwardDirection; }

		NODISCARD float GetNearPlane() const { return m_NearClip; }
		NODISCARD float GetFarPlane() const { return m_FarClip; }

		NODISCARD float GetLeft() const { return m_Parameters.Left; }
		NODISCARD float GetRight() const { return m_Parameters.Right; }
		NODISCARD float GetTop() const { return m_Parameters.Top; }
		NODISCARD float GetBottom() const { return m_Parameters.Bottom; }

		NODISCARD float GetFOV() const { return m_VerticalFOV; }

		NODISCARD float GetRotationSpeed();

		//NODISCARD bool IsInsideFrustum(const Math::AABB& boundingBox) const;

		NODISCARD void SetCameraTarget(const V3& target) { m_TargetPos = target; }

		bool FrustumCulling = true;

	private:
		bool NormalCameraControls(const V2& delta, float ts);
		bool OrbitCameraControls(const V2& delta, float ts);

		void RecalculateProjection();
		void RecalculateView();

		//Math::Frustum m_Frustum;

		Mat4 m_Projection{ 1.0f };
		Mat4 m_View{ 1.0f };
		Mat4 m_InverseProjection{ 1.0f };
		Mat4 m_InverseView{ 1.0f };

		float m_VerticalFOV = 45.0f;
		float m_NearClip = 0.1f;
		float m_FarClip = 100.0f;

		V3 m_TargetPos{ 0.0f, 0.0f, 0.0f };
		float m_TargetDist{ 0.0f };

		V3 m_Position{ 0.0f, 0.0f, 0.0f };
		V3 m_ForwardDirection{ 0.0f, 0.0f, 0.0f };

		V2 m_LastMousePosition{ 0.0f, 0.0f };

		u32 m_ViewportWidth = 0, m_ViewportHeight = 0;

		CameraParameters m_Parameters;
	};
}