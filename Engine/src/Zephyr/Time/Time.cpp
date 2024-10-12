/*
MIT License

Copyright (c) 2023 Víctor Falcón Zaro

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

#include "pch.h"
#include "Time.h"

namespace Zephyr::Time
{
	namespace
	{
		float g_DeltaTime;

		std::chrono::time_point<std::chrono::steady_clock> g_LastTime;

		float g_TimeSinceStart = 0.0f;
	}

	float GetTimeSinceStart()
	{
		return g_TimeSinceStart;
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
	}
}
