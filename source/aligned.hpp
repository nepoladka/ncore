#pragma once

namespace ncore {
	template<typename part_t, typename count_t = unsigned __int64> struct aligned {
		part_t low, high, divided;
		count_t count;

		__forceinline constexpr __fastcall aligned(part_t number, part_t maximal) noexcept {
			low = number % maximal;
			high = number - low;
			count = high / maximal;
			divided = high / count;
		}

		__forceinline constexpr auto __fastcall part() const noexcept {
			return divided;
		}
	};
}
