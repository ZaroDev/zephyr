#include "pch.h"
#include "Time.h"

namespace Zephyr::Time
{
	namespace
	{
		float g_DeltaTime;

		std::chrono::time_point<std::chrono::steady_clock> g_LastTime;

		float g_TimeSinceStart = 0.0f;
		u64 g_FrameCount = 0;
	}

	float GetTimeSinceStart()
	{
		return g_TimeSinceStart;
	}

	float GetAverageFrameTime()
	{
		return 0.f;
	}

	float GetAverageFPS()
	{
		return 0;
	}

	float GetDeltaTime()
	{
		return g_DeltaTime;
	}

	float GetFPS()
	{
		return 1000.0f / g_DeltaTime;
	}

	void StartTimeUpdate()
	{
		g_LastTime = std::chrono::high_resolution_clock::now();
	}

	void EndTimeUpdate()
	{
		const auto endTime = std::chrono::high_resolution_clock::now();
		const std::chrono::duration<float> duration = (endTime - g_LastTime);
		g_DeltaTime = duration.count() * 1000.0f;

		g_TimeSinceStart += duration.count();
		g_FrameCount++;
	}
}
