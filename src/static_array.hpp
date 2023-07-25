#pragma once
#include "defines.hpp"
#include <varargs.h>

namespace ncore {
	template<typename _t, size_t _arrayLength> class static_array {
	private:
		_t _values[_arrayLength];

	public:
		static __forceinline static_array* from_native(const _t* native) {
			return (static_array*)native;
		}

		__forceinline static_array(const _t data[_arrayLength] = nullptr, size_t size = _arrayLength) {
			for (size_t i = 0; i < _arrayLength; i++) {
				_values[i] = (data && i < size) ? data[i] : _t();
			}
		}

		__forceinline static_array(const _t data, ...) {
			va_list list;
			va_start(list, _arrayLength);

			for (size_t i = 0; i < _arrayLength; i++) {
				_values[i] = va_arg(list, _t);
			}

			va_end(list);
		}

		__forceinline _t* native() noexcept {
			return _values;
		}

		__forceinline const _t* native() const noexcept {
			return _values;
		}

		__forceinline _t* data() noexcept {
			return native();
		}

		__forceinline const _t* data() const noexcept {
			return native();
		}

		__forceinline size_t size() const noexcept {
			return _arrayLength;
		}

		__forceinline _t& operator[](size_t index) noexcept {
			return _values[index];
		}

		__forceinline const _t& operator[](size_t index) const noexcept {
			return _values[index];
		}

		__forceinline bool operator==(const static_array& second) const noexcept {
			for (size_t i = 0; i < _arrayLength; i++)
				if (_values[i] != second._values[i]) return false;

			return true;
		}

		__forceinline bool operator!=(const static_array& second) const noexcept {
			return !this->operator==(second);
		}
	};
}