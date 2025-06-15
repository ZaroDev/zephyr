/*
MIT License

Copyright (c) 2025 ZaroDev

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
#pragma once
#include <filesystem>
#include "Log.h"

#ifdef ENABLE_ASSERTS

// Alternatively we could use the same "default" message for both "WITH_MSG" and "NO_MSG" and
// provide support for custom formatting by concatenating the formatting string instead of having the format inside the default message
#define INTERNAL_ASSERT_IMPL(type, check, msg, ...) do { if(!(check)) { type##ERROR(msg, __VA_ARGS__); DEBUGBREAK(); } } while(0)
#define INTERNAL_ASSERT_WITH_MSG(type, check, ...) INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
#define INTERNAL_ASSERT_NO_MSG(type, check) INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

#define INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define INTERNAL_ASSERT_GET_MACRO(...) EXPAND_MACRO(INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, INTERNAL_ASSERT_WITH_MSG, INTERNAL_ASSERT_NO_MSG) )

// Currently accepts at least the condition and one additional parameter (the message) being optional
#define ASSERT(...) EXPAND_MACRO(INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__))
#define CORE_ASSERT(...) EXPAND_MACRO(INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(CORE_,__VA_ARGS__))
#else
#define ASSERT(...)
#define CORE_ASSERT(...)
#endif