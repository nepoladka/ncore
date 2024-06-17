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

#include "types.hpp"
