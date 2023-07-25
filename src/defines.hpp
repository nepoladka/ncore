#pragma once

#define CURRENT_PROCESS_HANDLE ((void*)(-1))

#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000ull
#endif

#ifndef M_RAD
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static constexpr const auto const __m_rad = 180 / M_PI;
#define M_RAD (__m_rad)
#endif

#define size_of_array(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))

#define get_procedure_t(RETURN, NAME, ...) RETURN(*NAME)(__VA_ARGS__)

#define null (0)

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

		using sbyte_t = i8_t;
		using byte_t = ui8_t;

		using ssize_t = i64_t;
		using size_t = ui64_t;

		using sindex_t = i64_t;
		using index_t = ui64_t;

		using offset_t = ui64_t;

		using address_t = void*;
	}

	using namespace types;
}
