#pragma once

extern"C" void* _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

#define __return_address _ReturnAddress()

#define CURRENT_PROCESS_HANDLE ((void*)(-1))
#define CURRENT_THREAD_HANDLE ((void*)(-2))

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000ui64
#endif

#define null (0)
#define NULL (0)

#define __b(NUM) size_t(NUM)				//byte
#define __kb(NUM) size_t(__b(1024) * NUM)	//kilo
#define __mb(NUM) size_t(__kb(1024) * NUM)	//mega
#define __gb(NUM) size_t(__mb(1024) * NUM)	//giga
#define __tb(NUM) size_t(__gb(1024) * NUM)	//tera
#define __pb(NUM) size_t(__tb(1024) * NUM)	//peta

#ifndef ncore_procedure
#define ncore_procedure(TYPE) __forceinline TYPE __fastcall
#endif

#define const_value(TYPE, NAME, VALUE) static constexpr const TYPE const NAME = { VALUE }
#define CONST_VALUE const_value

#ifndef M_PI
const_value(double, __m_pi, 3.14159265358979323846);
#define M_PI __m_pi
#endif

#ifndef M_180_PI
const_value(double, __m_180_pi, 180.0 / M_PI);
#define M_180_PI (__m_180_pi)
#endif

#ifndef M_PI_180
const_value(double, __m_pi_180, M_PI / 180.0);
#define M_PI_180 (__m_pi_180)
#endif

#ifndef __max
#define __max(A, B) (((A) > (B)) ? (A) : (B))
#endif

#ifndef __min
#define __min(A, B) (((A) < (B)) ? (A) : (B))
#endif

#ifndef __minmax
#define __minmax(MIN, VAL, MAX) __max(__min(VAL, MAX), MIN)
#endif

#ifndef __normalize
//#define __normalize(MIN, VAL, MAX) (((VAL) - (MIN)) / ((MAX) - (MIN)))
#define __normalize(MIN, VAL, MAX) ((VAL) < (MIN) ? (0) : ((VAL > MAX) ? (1) : (((VAL) - (MIN)) / ((MAX) - (MIN)))))
#endif

#ifndef __absolute
#define __absolute(VAL) (((VAL) >= NULL) ? (VAL) : (-VAL))
#endif

#define align_up(VALUE, ALIGNMENT) (((VALUE) + (ALIGNMENT) - 1) & ~((ALIGNMENT) - 1))
#define align_down(VALUE, ALIGNMENT) ((VALUE) & ~((ALIGNMENT) - 1))
#define align_page_up(X) align_up(unsigned __int64(X), PAGE_SIZE)
#define align_page_down(X) align_down(unsigned __int64(X), PAGE_SIZE)
#define PAGE_ROUND_UP align_page_up
#define PAGE_ROUND_DOWN align_page_down

#define sizeofarr(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))
#define size_of_array sizeofarr

#define proc_type_conv(CONVERSION, RETURN, NAME, ...) RETURN(CONVERSION* NAME)(__VA_ARGS__)
#define proc_type(RETURN, NAME, ...) RETURN(*NAME)(__VA_ARGS__)
#define get_procedure_t proc_type

#define stringify(VALUE) #VALUE
#define STRINGIFY stringify

#define include_library(NAME) _Pragma(stringify(comment(lib, NAME)))

#define begin_unaligned _Pragma("pack(push, 1)")
#define end_unaligned _Pragma("pack(pop)")
#define BEGIN_UNALIGNED begin_unaligned
#define END_UNALIGNED end_unaligned

#define typeinf(TYPE) &typeid(TYPE)
#define typeof decltype

#define bit_field(NAME) ncore::types::bit_t NAME : 1
#define __bit(NAME) bool NAME : 1

#define __property(GETTER, SETTER) __declspec(property(get = ##GETTER, put = ##SETTER))
#define __public public:
#define __private private:

#define __tryif(STATE) if (STATE) __try
#define __catch __except(true)
#define __endtry __catch { }

#define __extern extern "C"
#define __extern_export __extern __declspec(dllexport)
#define __extern_import __extern __declspec(dllimport)

#define __likely [[likely]]
#define __unlikely [[unlikely]]

#define __deprecated(WHY) [[deprecated(WHY)]]

#define __FILE_UNIQUE__ (__FILE__[sizeof(__FILE__) - 1] + __FILE__[sizeof(__FILE__) - 2] * 10 + __FILE__[sizeof(__FILE__) - 3] * 100 + __FILE__[sizeof(__FILE__) - 4] * 1000 + __FILE__[sizeof(__FILE__) - 5] * 10000 + __FILE__[sizeof(__FILE__) - 6] * 100000)
#define __TIME_UNIQUE__ (__TIME__[7] + __TIME__[6] * 10 + __TIME__[4] * 100 + __TIME__[3] * 1000 + __TIME__[1] * 10000 + __TIME__[0] * 100000)
#define __DATE_UNIQUE__ (__DATE__[4] + __DATE__[5] * 10 + __DATE__[7] * 100 + __DATE__[8] * 1000 + __DATE__[9] * 10000 + __DATE__[10] * 100000)
#define __BUILD_UNIQUE__ (__TIME_UNIQUE__ ^ __DATE_UNIQUE__)
#define __LINE_UNIQUE__ (__FILE_UNIQUE__ ^ __LINE__)
#define __UNIQUE__ (__FILE_UNIQUE__ ^ __BUILD_UNIQUE__)

/*
static void sample() noexcept {
	if constexpr (__isuniq(__COUNTER__)) {
		printf("build number is - 1\n");
	}
	else if constexpr (__isuniq(__COUNTER__)) {
		printf("build number is - 2\n");
	}
	else if constexpr (__isuniq(__COUNTER__)) {
		printf("build number is - 3\n");
	}
	else if constexpr (__isuniq(__COUNTER__)) {
		printf("build number is - 4\n");
	}
	else if constexpr (__isuniq(__COUNTER__)) {
		printf("build number is - 5\n");
	}
	else if constexpr (__isuniq(__COUNTER__)) {
		printf("build number is - 6\n");
	}
	else {
		printf("build number is - 0\n");
	}
}
*/
#define __isuniq(NUMBER) ((__TIME_UNIQUE__ % (NUMBER + (3 * (NUMBER < 3)))) == 0)

#include "types.hpp"

namespace ncore {
	static __forceinline constexpr bool is_little_endian(ui64_t value) noexcept {
		const auto bytes = (const ui8_t*)(&value);
		return *bytes == (value & 0xFFui8);
	}

	static __forceinline constexpr bool is_big_endian(ui64_t value) noexcept {
		const auto bytes = (const ui8_t*)(&value);
		return *bytes != (value & 0xFFui8);
	}

	static __forceinline constexpr ui64_t swap_endian(ui64_t value) noexcept {
		return ((value & 0xFF00000000000000ui64) >> 56) |
			((value & 0x00FF000000000000ui64) >> 40) |
			((value & 0x0000FF0000000000ui64) >> 24) |
			((value & 0x000000FF00000000ui64) >> 8) |
			((value & 0x00000000FF000000ui64) << 8) |
			((value & 0x0000000000FF0000ui64) << 24) |
			((value & 0x000000000000FF00ui64) << 40) |
			((value & 0x00000000000000FFui64) << 56);
	}

	static __forceinline constexpr ui32_t swap_endian(ui32_t value) noexcept {
		return ((value & 0xFF000000ui32) >> 24) |
			((value & 0x00FF0000ui32) >> 8) |
			((value & 0x0000FF00ui32) << 8) |
			((value & 0x000000FFui32) << 24);
	}

	static __forceinline constexpr ui16_t swap_endian(ui16_t value) noexcept {
		return ((value & 0xFF00ui16) >> 8) |
			((value & 0x00FFui16) << 8);
	}

	static __forceinline constexpr ui8_t swap_endian(ui8_t value) noexcept {
		return value;
	}
}
