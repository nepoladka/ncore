#pragma once

namespace ncore {
	namespace types {
		template <typename _t, const _t _value> struct stored_const {
			static constexpr const _t const value = _value;
		};

		template<typename summary_t, typename half_t> union part_value {
			struct {
				half_t primary, secondary;
			};

			summary_t summary;

			__forceinline constexpr part_value(summary_t summary = summary_t()) noexcept {
				this->summary = summary;
			}

			__forceinline constexpr part_value(half_t primary, half_t secondary) noexcept {
				this->primary = primary;
				this->secondary = secondary;
			}

			__forceinline constexpr const auto const operator==(summary_t second) const noexcept {
				return summary == second;
			}

			__forceinline constexpr const auto const operator==(part_value second) const noexcept {
				return summary == second.summary;
			}

			__forceinline constexpr summary_t& operator=(summary_t value) noexcept {
				return summary = value;
			}

			__forceinline constexpr half_t& operator[](bool secondary) const noexcept {
				return secondary ? this->secondary : this->primary;
			}

			__forceinline constexpr half_t& operator[](bool secondary) noexcept {
				return secondary ? this->secondary : this->primary;
			}
		};

		using i8_t = __int8;
		using i16_t = __int16;
		using i32_t = __int32;
		using i64_t = __int64;
		using i8_p = i8_t*;
		using i16_p = i16_t*;
		using i32_p = i32_t*;
		using i64_p = i64_t*;

		using ui8_t = unsigned __int8;
		using ui16_t = unsigned __int16;
		using ui32_t = unsigned __int32;
		using ui64_t = unsigned __int64;
		using ui8_p = ui8_t*;
		using ui16_p = ui16_t*;
		using ui32_p = ui32_t*;
		using ui64_p = ui64_t*;

		using pi16_t = part_value<i16_t, i8_t>;
		using pi32_t = part_value<i32_t, i16_t>;
		using pi64_t = part_value<i64_t, i32_t>;
		using pi16_p = pi16_t*;
		using pi32_p = pi32_t*;
		using pi64_p = pi64_t*;

		using pui16_t = part_value<ui16_t, ui8_t>;
		using pui32_t = part_value<ui32_t, ui16_t>;
		using pui64_t = part_value<ui64_t, ui32_t>;
		using pui16_p = pui16_t*;
		using pui32_p = pui32_t*;
		using pui64_p = pui64_t*;

		using byte_t = ui8_t;
		using byte_p = byte_t*;

		using sbyte_t = i8_t;
		using sbyte_p = sbyte_t*;

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
		using loffset_t = ui32_t;

		using address_t = void*;
		using address_p = address_t*;

		using bit_t = bool;

		union s8_t {
			ui8_t summary;

			struct {
				bit_t b0 : 1;
				bit_t b1 : 1;
				bit_t b2 : 1;
				bit_t b3 : 1;
				bit_t b4 : 1;
				bit_t b5 : 1;
				bit_t b6 : 1;
				bit_t b7 : 1;
			};

			__forceinline constexpr const void operator=(ui8_t value) noexcept {
				summary = value;
			}
		};

		using s16_t = part_value<ui16_t, s8_t>;
		using s32_t = part_value<ui32_t, s16_t>;
		using s64_t = part_value<ui64_t, s32_t>;

		template<typename _t, bool _minEquals = true, bool _maxEquals = true> struct limit {
			_t min, max;
			limit* second;
			char second_comparsion_type;

			__forceinline constexpr limit(const _t& min = _t(), const _t& max = _t(), limit* second = nullptr, char second_comparsion_type = '&') noexcept {
				this->min = min;
				this->max = max;
				this->second = second;
				this->second_comparsion_type = second_comparsion_type;
			}

			__forceinline constexpr auto in_range(const _t& value, bool check_second = true) const noexcept {
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

			__forceinline constexpr const auto difference() const noexcept {
				return max - min;
			}

			__forceinline constexpr auto& operator[](bool get_max) noexcept {
				return get_max ? max : min;
			}

			__forceinline constexpr const auto& operator[](bool get_max) const noexcept {
				return get_max ? max : min;
			}

			__forceinline constexpr auto begin() noexcept {
				return min;
			}

			__forceinline constexpr auto end() noexcept {
				return max;
			}

			__forceinline constexpr const auto begin() const noexcept {
				return min;
			}

			__forceinline constexpr const auto end() const noexcept {
				return max;
			}
		};

		template<typename _t> using bound = limit<_t, true, true>;
		template<typename _t> using range = limit<_t, false, false>;

#ifdef get_procedure_t
		template<typename part_t, typename data_t = void*> struct aligned {
			using callback_t = get_procedure_t(void, , index_t index, part_t value, data_t data);

			part_t low, high, divided;
			count_t count;

			__forceinline constexpr aligned(part_t value, part_t step, callback_t callback, data_t data = data_t()) noexcept {
				if (!callback) return;

				count = count_t();
				divided = step;

				auto remaining = value;
				for (; remaining > step; remaining -= step, high += step, count++) {
					callback(count, step, data);
				}

				if (low = remaining) {
					callback(count, remaining, data);
				}
			}

			__forceinline constexpr aligned(part_t number, part_t maximal) noexcept {
				if (maximal) {
					low = number % maximal;
					high = number - low;
					count = high / maximal;
					divided = high / count;
					return;
				}

				low = number;
				high = part_t();
				count = null;
				divided = null;
			}

			__forceinline constexpr auto part() const noexcept {
				return divided;
			}
		};
#endif
	}

	using namespace types;
}

#ifdef NCORE_MAKE_TYPES_GLOBAL
using namespace ncore::types;
#endif