#pragma once
#include "defines.hpp"
#include <varargs.h>
#include <vector>

namespace ncore {
	template<typename _t, size_t _capacity> class static_array {
	protected:
		_t _values[_capacity];

	public:
		static __forceinline static_array* from_native(const _t* native) noexcept {
			return (static_array*)native;
		}

		__forceinline constexpr static_array(const _t data[_capacity] = nullptr, count_t size = _capacity) noexcept {
			for (count_t i = 0; i < _capacity; i++) {
				_values[i] = (data && i < size) ? data[i] : _t();
			}
		}

		__forceinline static_array(const _t data, ...) noexcept {
			va_list list;
			va_start(list, _capacity);

			for (size_t i = 0; i < _capacity; i++) {
				_values[i] = va_arg(list, _t);
			}

			va_end(list);
		}

		__forceinline constexpr _t* native() noexcept {
			return _values;
		}

		__forceinline constexpr const _t* native() const noexcept {
			return _values;
		}

		__forceinline constexpr _t* data() noexcept {
			return native();
		}

		__forceinline constexpr const _t* data() const noexcept {
			return native();
		}

		__forceinline constexpr size_t size() const noexcept {
			return _capacity;
		}

		__forceinline constexpr size_t capacity() const noexcept {
			return _capacity;
		}

		__forceinline constexpr _t* begin() noexcept {
			return _values;
		}

		__forceinline constexpr const _t* begin() const noexcept {
			return _values;
		}

		__forceinline constexpr _t* end() noexcept {
			return _values + _capacity;
		}

		__forceinline constexpr const _t* end() const noexcept {
			return _values + _capacity;
		}

		__forceinline size_t length(_t ending = _t()) const noexcept {
			auto result = size_t(0);

			for (auto& item : _values) {
				if (item == ending) break;
				result++;
			}

			return result;
		}

		__forceinline constexpr _t& at(size_t index) noexcept {
			return _values[index];
		}

		__forceinline constexpr const _t& at(size_t index) const noexcept {
			return _values[index];
		}

		__forceinline constexpr const void copy(static_array& destination) const noexcept {
			auto length = min(capacity(), destination.capacity());
			for (auto i = 0; i < length; i++) {
				destination._values[i] = _values[i];
			}
		}

		__forceinline constexpr _t& operator[](size_t index) noexcept {
			return at(index);
		}

		__forceinline constexpr const _t& operator[](size_t index) const noexcept {
			return at(index);
		}

		__forceinline bool operator==(const static_array& second) const noexcept {
			for (size_t i = 0; i < _capacity; i++)
				if (_values[i] != second._values[i]) return false;

			return true;
		}

		__forceinline bool operator!=(const static_array& second) const noexcept {
			return !this->operator==(second);
		}

		__forceinline constexpr _t& operator*() noexcept {
			return _values[0];
		}
	};

	template<typename _t> struct static_element_t {
		bool filled = false;
		_t value = _t();

		__forceinline constexpr const bool operator==(const static_element_t& second) {
			return value == second.value;
		}

		__forceinline constexpr const bool operator==(const _t& value) {
			return this->value == value;
		}

		__forceinline constexpr const bool empty() {
			return !filled;
		}
	};

	template<typename _t, size_t _capacity, class base_t = static_array<static_element_t<_t>, _capacity>> class static_vector : public base_t {
	public:
		__forceinline constexpr static_vector() = default;

		__forceinline constexpr static_vector(const std::vector<_t>& vec) : static_vector(vec.data(), vec.size()) { return; }

		__forceinline constexpr static_vector(const static_vector& second) noexcept {
			*this = second;
		}

		__forceinline constexpr static_vector(const _t* values, size_t count) noexcept {
			auto index = size_t(0);
			for (auto& element : base_t::_values) {
				if (index >= count) break;
				element = { true, values[index++] };
			}
		}

		__forceinline constexpr const size_t capacity() const noexcept {
			return _capacity;
		}

		__forceinline constexpr const size_t size() const noexcept {
			auto count = size_t();
			for (auto& element : base_t::_values) {
				count += element.filled;
			}
			return count;
		}

		__forceinline constexpr const size_t length() const noexcept {
			return size();
		}

		__forceinline constexpr const size_t count() const noexcept {
			return size();
		}

		__forceinline constexpr _t& at(size_t idx) noexcept {
			return base_t::at(idx).value;
		}

		__forceinline constexpr const _t& at(size_t idx) const noexcept {
			return base_t::at(idx).value;
		}

		__forceinline constexpr static_vector resort() const noexcept {
			auto result = static_vector();

			for (size_t i = 0, j = 0; i < _capacity; i++) {
				auto& element = base_t::_values[i];
				if (element.filled) {
					result._values[j++] = element;
				}
			}

			return result;
		}

		__forceinline constexpr const std::vector<_t> vec() const noexcept {
			auto result = std::vector<_t>();
			for (auto& element : base_t::_values) {
				if (element.filled) {
					result.push_back(element.value);
				}
			}
			return result;
		}

		__forceinline constexpr const bool add(const _t& value, size_t* _index = nullptr) noexcept {
			if (!_index) {
				auto index_buffer = size_t();
				_index = &index_buffer;
			}

			for (auto& i = *_index = 0; i < _capacity; i++) {
				auto& element = base_t::_values[i];
				if (element.filled) continue;

				element = { true, value };

				return true;
			}

			return false;
		}

		__forceinline constexpr const bool erase(size_t idx) noexcept {
			if (idx > _capacity || idx < 0) return false;

			auto& element = base_t::_values[idx];
			auto filled = element.filled;
			if (filled) {
				*static_array<byte_t, sizeof(static_element_t<_t>)>::from_native((const byte_t*)&element) = nullptr;
			}

			return filled;
		}

		__forceinline constexpr const bool search(const _t& pattern, size_t* _index = nullptr) const noexcept {
			for (auto i = size_t(0); i < _capacity; i++) {
				auto& element = base_t::_values[i];
				if (!element.filled) continue;

				if (element.value != pattern) continue;

				if (_index) {
					*_index = i;
				}

				return true;
			}
			
			return false;
		}

		__forceinline constexpr _t& operator[](size_t idx) noexcept {
			return base_t::operator[](idx).value;
		}

		__forceinline constexpr const _t& operator[](size_t idx) const noexcept {
			return base_t::operator[](idx).value;
		}

		__forceinline constexpr void operator=(const static_vector& second) noexcept {
			for (size_t i = 0; i < _capacity; i++) {
				base_t::_values[i] = second._values[i];
			}
		}
	};
}
