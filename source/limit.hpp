#pragma once

namespace ncore {
	template<typename _t, bool _minEquals = true, bool _maxEquals = true> struct limit {
		_t min, max;
		limit* second;
        char second_comparsion_type;

		__forceinline constexpr limit(const _t& min = _t(), const _t& max = _t(), limit* second = nullptr, char second_comparsion_type = '&') {
			this->min = min;
			this->max = max;
			this->second = second;
            this->second_comparsion_type = second_comparsion_type;
		}

		__forceinline constexpr bool in_range(const _t& value, bool check_second = true) const noexcept {
			auto result = false;

			if constexpr (_minEquals) {
				result = *((_t*)&value) >= min;
			}
			else {
				result = *((_t*)&value) > min;
			}
			if (!result) return false;

			if constexpr (_maxEquals) {
				result = *((_t*)&value) <= max;
			}
			else {
				result = *((_t*)&value) < max;
			}
			if (!result) return false;

			if (second && check_second) switch (second_comparsion_type) {
			case '&': default: result = result && second->in_range(value); break;
			case '|': result = result || second->in_range(value); break;
			}

			return result;
		}

		__forceinline _t& operator[](bool get_max) noexcept { return get_max ? max : min; }
	};
}
