#pragma once

extern"C" void* _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)

#define __return_address _ReturnAddress()

#define CURRENT_PROCESS_HANDLE ((void*)(-1))
#define CURRENT_THREAD_HANDLE ((void*)(-2))

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000ull
#endif

#define null (0)

#ifndef ncore_procedure
#define ncore_procedure(TYPE) __forceinline TYPE __fastcall
#endif

#define const_value(TYPE, NAME, VALUE) static constexpr const TYPE const NAME = { VALUE }
#define CONST_VALUE const_value

#ifndef M_RAD
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

const_value(double, __m_rad, 180.0 / M_PI);
#define M_RAD (__m_rad)
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
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

#define __tryif(STATE) if (STATE) __try
#define __catch __except(true)
#define __endtry __catch { }
#define __extern extern "C"
#define __extern_export __extern __declspec(dllexport)
#define __extern_import __extern __declspec(dllimport)

namespace ncore {
	namespace types {
		using i8_t = __int8;
		using i16_t = __int16;
		using i32_t = __int32;
		using i64_t = __int64;

		using ui8_t = unsigned __int8;
		using ui16_t = unsigned __int16;
		using ui32_t = unsigned __int32;
		using ui64_t = unsigned __int64;

		using ui8_p = ui8_t*;
		using ui16_p = ui16_t*;
		using ui32_p = ui32_t*;
		using ui64_p = ui64_t*;

		using sbyte_t = i8_t;
		using sbyte_p = sbyte_t*;

		using byte_t = ui8_t;
		using byte_p = byte_t*;

		using lssize_t = i32_t;
		using lsize_t = ui32_t;

		using ssize_t = i64_t;
		using ssize_p = ssize_t*;

		using size_t = ui64_t;
		using size_p = size_t*;

		using scount_t = i64_t;
		using count_t = ui64_t;

		using sindex_t = i64_t;
		using index_t = ui64_t;

		using soffset_t = i64_t;
		using offset_t = ui64_t;

		using address_t = void*;
		using address_p = address_t*;
	}

	using namespace types;
}
