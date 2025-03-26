#pragma once
#include "PlatformDetection.h"
#include <memory>
#include <filesystem>
#include <string>
#include <fstream>


#ifdef DEBUG
#if defined(PLATFORM_WINDOWS)
#define DEBUGBREAK() __debugbreak()
#elif defined(PLATFORM_LINUX)
#inlcude <singal.h>
#define DEBUGBREAK() raise(SIGTRAP)
#else
#error "Platform doesn't support debug break!"
#endif

#else
#define DEBUGBREAK()
#endif

#if _HAS_NODISCARD
#define NODISCARD [[nodiscard]]
#else // ^^^ CAN HAZ [[nodiscard]] / NO CAN HAZ [[nodiscard]] vvv
#define NODISCARD
#endif // _HAS_NODISCARD

#define INLINE inline
#define FORCE_INLINE __forceinline

#define EXPAND_MACRO(x) x
#define STRINGIFY_MACRO(x) #x

#define BIT(x) (1 << x)

#define BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }



#ifndef DEFAULT_COPY
#define DEFAULT_COPY(T) \
	T(const T&) = default; \
	T& operator=(const T&) = default;
#endif

#ifndef DEFAULT_MOVE
#define DEFAULT_MOVE(T) \
	T(T&&) noexcept = default; \
	T& operator=(T&&) = default;
#endif


#ifndef DEFAULT_MOVE_AND_COPY
#define DEFAULT_MOVE_AND_COPY(T) \
		DEFAULT_MOVE(T) \
		DEFAULT_COPY(T)
#endif


#ifndef DISABLE_COPY
#define DISABLE_COPY(T) \
	T(const T&) = delete; \
	T& operator=(const T&) = delete; 
#endif

#ifndef DISABLE_MOVE
#define DISABLE_MOVE(T) \
	T(T&&) = delete; \
	T& operator=(T&&) = delete;
#endif

#ifndef DISABLE_MOVE_AND_COPY
#define DISABLE_MOVE_AND_COPY(T) \
	DISABLE_MOVE(T) \
	DISABLE_COPY(T) 
#endif

namespace Zephyr
{
	typedef std::filesystem::path Path;
	typedef std::string String;
	typedef std::string_view StrView;
	typedef std::ifstream FileStream;
	typedef std::stringstream StringStream;

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args) 
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}
	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

	template<typename C, typename P>
	constexpr Ref<C> DynCast(Ref<P> ref)
	{
		return std::dynamic_pointer_cast<C>(ref);
	}

	template<typename C, typename P>
	constexpr Ref<C> Cast(Ref<P> ref)
	{
		return std::static_pointer_cast<C>(ref);
	}
}