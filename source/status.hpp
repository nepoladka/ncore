#pragma once

namespace ncore {
	template<typename summary_t, typename half_t> union status_t {
		struct {
			half_t primary, secondary;
		};

		summary_t summary;

		__forceinline constexpr status_t(summary_t summary = summary_t()) noexcept {
			this->summary = summary;
		}

		__forceinline constexpr status_t(half_t primary, half_t secondary) noexcept {
			this->primary = primary;
			this->secondary = secondary;
		}

		__forceinline constexpr const bool const operator==(summary_t second) const noexcept {
			return summary == second;
		}

		__forceinline constexpr const bool const operator==(status_t second) const noexcept {
			return summary == second.summary;
		}

		/*__forceinline constexpr summary_t& operator=() const noexcept {
			return summary;
		}*/

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

	union s8_t {
		unsigned __int8 summary;

		struct {
			bool b0 : 1;
			bool b1 : 1;
			bool b2 : 1;
			bool b3 : 1;
			bool b4 : 1;
			bool b5 : 1;
			bool b6 : 1;
			bool b7 : 1;
		};

		__forceinline constexpr const void operator=(unsigned __int8 value) noexcept {
			summary = value;
		}
	};

	using s16_t = status_t<unsigned __int16, s8_t>;
	using s32_t = status_t<unsigned __int32, s16_t>;
	using s64_t = status_t<unsigned __int64, s32_t>;
}
