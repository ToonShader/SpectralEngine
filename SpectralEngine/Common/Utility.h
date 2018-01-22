#pragma once

#include <Windows.h>

#ifndef SPECTRAL_ASSERTS
#define	SPECTRAL_ASSERTS

#ifndef RELEASE
#define ASSERT(exp, outstr)			\
if (!static_cast<bool>(exp))		\
{									\
    OutputDebugString(L"\nAssertion failed in " __FILE__ " @ " __LINE__ "\n"); \
    OutputDebugString(L"\'" #exp "\' is false\n"); \
    __debugbreak();					\
}

#define ASSERT_HR(hr, outstr)		\
if (FAILED(hr))						\
{									\
    OutputDebugString(L"\nHRESULT failed in " __FILE__ " @ " __LINE__ "\n"); \
    OutputDebugString(L"HRESULT = " #hr "\n"); \
    __debugbreak();					\
}

#else
#define ASSERT(exp, outstr) (exp)
#define ASSERT_HR(hr, outstr) (hr)

#endif // RELEASE

#endif // SPECTRAL_ASSERTS