#pragma once

namespace ncore {
	template<typename key_t, typename value_t> class pair {
	private:
		key_t _key;
		value_t _value;

	public:
		__forceinline pair(const key_t& key, const value_t& value) noexcept {
			_key = key;
			_value = value;
		}

		__forceinline auto& key() noexcept {
			return _key;
		}

		__forceinline const auto& key() const noexcept {
			return _key;
		}

		__forceinline auto& value() noexcept {
			return _value;
		}

		__forceinline const auto& value() const noexcept {
			return _value;
		}
	};
}
