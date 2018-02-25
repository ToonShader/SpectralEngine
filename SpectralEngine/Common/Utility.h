#pragma once

#include <Windows.h>

#ifndef SPECTRAL_ASSERTS
#define	SPECTRAL_ASSERTS

#ifndef RELEASE
// Passthrough macros so we can use __FILE__ and __LINE__ in strings
#define L1_PT(x) #x
#define L2_PT(x) L1_PT(x)
#define ASSERT(exp, outstr)			\
if (!static_cast<bool>(exp))		\
{									\
    OutputDebugString(L"\nAssertion failed in " L2_PT(__FILE__) " @ " L2_PT(__LINE__) "\n"); \
    OutputDebugString(L"\'" #exp "\' is false\n"); \
	OutputDebugString(outstr "\n");	\
    __debugbreak();					\
}

#define ASSERT_HR(hr/*, outstr*/)		\
if (FAILED(hr))						\
{									\
    OutputDebugString(L"\nHRESULT failed in " L2_PT(__FILE__) " @ " L2_PT(__LINE__) "\n"); \
    OutputDebugString(L"HRESULT = " #hr "\n"); \
	/*OutputDebugString(outstr L"\n");*/	\
    __debugbreak();					\
}

#else
#define ASSERT(exp, outstr) (exp)
#define ASSERT_HR(hr, outstr) (hr)

#endif // RELEASE

#endif // SPECTRAL_ASSERTS