﻿#pragma once
#include <Zephyr/Core/Base.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <functional>


// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

#undef ERROR

namespace Zephyr
{
	enum LogLevel 
	{
		TRACE,
		INFO,
		WARN,
		ERR,
		CRITICAL,
		LAST
	};

	typedef std::function<void(LogLevel, Zephyr::String)> LogCallback;

	class Log
	{
	public:
		static void Init();
		static void SetLogCallback(LogCallback callback)
		{
			s_Callback = callback;
		}

		static void AddLog(LogLevel level)
		{
			const std::vector<spdlog::sink_ptr>& sinks = s_ClientLogger.get()->sinks();
			if (s_Callback)
			{
				s_Callback(level, Zephyr::String(sinks[0].get()->get_last()));
			}
		}

		static void AddCoreLog(LogLevel level)
		{
			const std::vector<spdlog::sink_ptr>& sinks = s_CoreLogger.get()->sinks();
			if (s_Callback)
			{
				s_Callback(level, Zephyr::String(sinks[0].get()->get_last()));
			}
		}

		inline static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
		static LogCallback s_Callback;
	};
}

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& vector)
{
	return os << glm::to_string(vector);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& matrix)
{
	return os << glm::to_string(matrix);
}

template<typename OStream, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, glm::qua<T, Q> quaternion)
{
	return os << glm::to_string(quaternion);
}

// Core log macros
#define CORE_TRACE(...)    ::Zephyr::Log::GetCoreLogger()->trace(__VA_ARGS__); ::Zephyr::Log::AddCoreLog(Zephyr::LogLevel::TRACE)
#define CORE_INFO(...)     ::Zephyr::Log::GetCoreLogger()->info(__VA_ARGS__); ::Zephyr::Log::AddCoreLog(Zephyr::LogLevel::INFO)
#define CORE_WARN(...)     ::Zephyr::Log::GetCoreLogger()->warn(__VA_ARGS__); ::Zephyr::Log::AddCoreLog(Zephyr::LogLevel::WARN)
#define CORE_ERROR(...)    ::Zephyr::Log::GetCoreLogger()->error(__VA_ARGS__); ::Zephyr::Log::AddCoreLog(Zephyr::LogLevel::ERR)
#define CORE_CRITICAL(...) ::Zephyr::Log::GetCoreLogger()->critical(__VA_ARGS__); ::Zephyr::Log::AddCoreLog(Zephyr::LogLevel::CRITICAL)

// Client log macros
#define TRACE(...)         ::Zephyr::Log::GetClientLogger()->trace(__VA_ARGS__); ::Zephyr::Log::AddLog(Zephyr::LogLevel::TRACE)
#define INFO(...)          ::Zephyr::Log::GetClientLogger()->info(__VA_ARGS__); ::Zephyr::Log::AddLog(Zephyr::LogLevel::INFO)
#define WARN(...)          ::Zephyr::Log::GetClientLogger()->warn(__VA_ARGS__); ::Zephyr::Log::AddLog(Zephyr::LogLevel::WARN)
#define ERROR(...)         ::Zephyr::Log::GetClientLogger()->error(__VA_ARGS__); ::Zephyr::Log::AddLog(Zephyr::LogLevel::ERR)
#define CRITICAL(...)      ::Zephyr::Log::GetClientLogger()->critical(__VA_ARGS__); ::Zephyr::Log::AddLog(Zephyr::LogLevel::CRITICAL)