#pragma once
#include <Zephyr/Core/Base.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>


// This ignores all warnings raised inside External headers
#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#pragma warning(pop)

#undef ERROR

namespace Zephyr
{
	enum LogLevel {
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

			s_Callback(level, );
		}

	
		static void AddCoreLog(LogLevel level)
		{
			const std::vector<spdlog::sink_ptr>& sinks = s_CoreLogger.get()->sinks();

			s_Callback(level, Zephyr::String(sinks[0].get()->get_last()));
		}

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
#define CORE_TRACE(...)    ::Zephyr::Log::AddCoreLog(LogLevel::TRACE, __VA_ARGS__)
#define CORE_INFO(...)     ::Zephyr::Log::AddCoreLog(LogLevel::INFO,__VA_ARGS__)
#define CORE_WARN(...)     ::Zephyr::Log::AddCoreLog(LogLevel::WARN,__VA_ARGS__)
#define CORE_ERROR(...)    ::Zephyr::Log::AddCoreLog(LogLevel::ERROR,__VA_ARGS__)
#define CORE_CRITICAL(...) ::Zephyr::Log::AddCoreLog(LogLevel::CRITICAL,__VA_ARGS__)

// Client log macros
#define TRACE(...)         ::Zephyr::Log::AddLog(LogLevel::TRACE, __VA_ARGS__)
#define INFO(...)          ::Zephyr::Log::AddLog(LogLevel::INFO,__VA_ARGS__)
#define WARN(...)          ::Zephyr::Log::AddLog(LogLevel::WARN,__VA_ARGS__)
#define ERROR(...)         ::Zephyr::Log::AddLog(LogLevel::ERROR,__VA_ARGS__)
#define CRITICAL(...)      ::Zephyr::Log::AddLog(LogLevel::CRITICAL,__VA_ARGS__)