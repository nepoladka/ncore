#pragma once
#define NCORE_ENUMERATION_NO_BINARY
#include "enumeration.hpp"

#include <vector>

namespace ncore {
	namespace utils {
		template<typename _t> static __forceinline constexpr void copy(_t* dst, const _t* src, count_t count) noexcept {
			for (index_t i = 0; i < count; ++i) {
				dst[i] = src[i];
			}
		}

		template<typename _t> static __forceinline constexpr void move(_t* dst, const _t* src, count_t count) noexcept {
			if (dst < src) {
				for (index_t i = 0; i < count; ++i) {
					dst[i] = src[i];
				}
			}
			else {
				for (index_t i = count; i > 0; --i) {
					dst[i - 1] = src[i - 1];
				}
			}
		}

		template<typename _t> static __forceinline constexpr void fill(_t* start, _t* end, const _t& value) noexcept {
			for (auto ptr = start; ptr != end; ++ptr) {
				*ptr = value;
			}
		}

		template<bool _right, typename _t> static __forceinline constexpr void shift(_t* array, count_t capacity, count_t size, index_t index, count_t count, bool erase, _t** _out = nullptr, count_t* _out_count = nullptr) noexcept {
			if (index >= capacity) {
				index = capacity - 1;
			}
			else if (index < 0) {
				index = 0;
			}




			/*
			
			
			
			1 2 3 4 5
		  1 2 3 _ 4 5      //left  | index = 2 | count = 1 |
			1 2 _ 3 4 5    //right | index = 2 | count = 1 |
			
			
			
			
			*/




			auto overflow = ssize_t(count - (capacity - size));
			if (overflow < 0) {
				overflow = 0;
			}

			if (_out_count) {
				*_out_count = overflow;
			}

			if (overflow && _out) {
				*_out = new _t[overflow];

				if constexpr (_right) {
					utils::copy(*_out, array + size - overflow, overflow);
				}
				else {
					utils::copy(*_out, array + index - overflow, overflow);
				}
			}

			if constexpr (_right) {
				if (index + count > size) {
					count = size - index;
				}

				utils::move(array + index + count, array + index, size - index - count);
				if (erase) {
					utils::fill(array + index, array + index + count, _t());
				}
			}
			else {
				if (index < count) {
					count = index;
				}

				utils::move(array + index - count, array + index, count);
				if (erase) {
					utils::fill(array + index, array + index + count, _t());
				}
			}
		}
	}

	using namespace utils;

	namespace types {
		template<typename _t> class collection : public std::vector<_t> {
		public:
			using base_t = std::vector<_t>;
			using base_t::vector;

			enum : index_t { npos = -1 };

			template<typename data_t, typename element_t> using enumeration_procedure_t = get_procedure_t(ncore::enumeration::return_t, , index_t, element_t, data_t);
			using comparison_procedure_t = get_procedure_t(int, , const _t& left, const _t& right);
			using native_comparison_procedure_t = _CoreCrtNonSecureSearchSortCompareFunction;

			/*__forceinline constexpr collection() noexcept : base_t({ }) {
				return;
			}

			__forceinline constexpr collection(const std::vector<_t>& vector) noexcept : base_t(vector) {
				return;
			}

			__forceinline constexpr collection(std::initializer_list<_t> list) noexcept : base_t(list) {
				return;
			}

			__forceinline constexpr collection(const _t* data, const count_t count) noexcept : base_t(data, data + count) {
				return;
			}

			__forceinline constexpr collection(_t* data, count_t count) noexcept : base_t(data, data + count) {
				return;
			}

			__forceinline constexpr collection(count_t count, const _t& value = _t()) noexcept : base_t(count, value) {
				return;
			}*/

			__forceinline constexpr auto& vector() noexcept {
				return *(std::vector<_t>*)this;
			}

			__forceinline constexpr const auto& vector() const noexcept {
				return *(std::vector<_t>*)this;
			}

			__forceinline constexpr auto count() const noexcept {
				return base_t::size();
			}

			template<typename data_t> __forceinline constexpr auto enumerate(enumeration_procedure_t<data_t, _t&> procedure, data_t data) noexcept {
				auto base = base_t::begin();
				if (procedure) __likely for (index_t i = 0; i < base_t::size(); i++) {
					if (procedure(i, *(base + i), data) == ncore::enumeration::return_t::stop) return i;
				}
				return index_t(npos);
			}

			template<typename data_t> __forceinline constexpr auto enumerate(enumeration_procedure_t<data_t, const _t&> procedure, data_t data) const noexcept {
				auto base = base_t::begin();
				if (procedure) __likely for (index_t i = 0; i < base_t::size(); i++) {
					if (procedure(i, *(base + i), data) == ncore::enumeration::return_t::stop) return i;
				}
				return index_t(npos);
			}

			__forceinline constexpr auto sort(comparison_procedure_t procedure) const noexcept {
				auto result = *this;

				if (procedure) {
					qsort(result.data(), result.size(), sizeof(_t), native_comparison_procedure_t(procedure));
				}

				return result;
			}

			__forceinline constexpr auto& reverse() noexcept {
				return std::reverse(base_t::begin(), base_t::end()), *this;
			}

			__forceinline constexpr auto reverse() const noexcept {
				auto result = *this;
				return result.reverse();
			}

			__forceinline constexpr auto skip_first(count_t count) const noexcept {
				auto result = collection();

				if (base_t::size() >= count) {
					result.insert(result.end(), base_t::begin() + count, base_t::end());
				}

				return result;
			}

			__forceinline constexpr auto skip_last(count_t count) const noexcept {
				auto result = collection();

				if (base_t::size() >= count) {
					result.insert(result.end(), base_t::begin(), base_t::end() - count);
				}

				return result;
			}

			__forceinline constexpr auto& exclude(base_t::const_iterator position) noexcept {
				return base_t::erase(position), *this;
			}

			__forceinline constexpr auto exclude(base_t::const_iterator position) const noexcept {
				auto result = *this;
				return result.exclude(position);
			}

			__forceinline constexpr auto& exclude(index_t position) noexcept {
				return base_t::erase(base_t::begin() + position), *this;
			}

			__forceinline constexpr auto exclude(index_t position) const noexcept {
				auto result = *this;
				return result.exclude(position);
			}

			template<bool _not = false, typename data_t = bool> __forceinline constexpr auto& exclude(const _t* elements, count_t count, enumeration_procedure_t<data_t, _t&> before_erase = nullptr, data_t data = data_t()) noexcept {
				auto base = base_t::begin();
				for (index_t i = 0; i < base_t::size(); i++) {
					auto current = base + i;
					auto& element = *current;

					auto in = false;
					for (auto item = elements, end = elements + count; item != end; item++) {
						if (in = (element == *item)) break;
					}

					if constexpr (_not) {
						if (in) continue;
					}
					else if (!in) continue;

					if (before_erase) {
						if (before_erase(i, element, data) == enumeration::return_t::skip) continue;
					}

					base_t::erase(current);
					i--;
				}

				return *this;
			}

			template<bool _not = false, typename data_t = bool> __forceinline constexpr auto& exclude(const collection& elements, enumeration_procedure_t<data_t, _t&> before_erase = nullptr, data_t data = data_t()) noexcept {
				return exclude<_not, data_t>(elements.data(), elements.count(), before_erase, data);
			}

			template<bool _not = false, typename data_t = bool> __forceinline constexpr auto exclude(const collection& elements, enumeration_procedure_t<data_t, const _t&> before_erase = nullptr, data_t data = data_t()) const noexcept {
				auto result = *this;
				return result.exclude<_not, data_t>(elements, before_erase, data);
			}

			template<typename data_t = bool> __forceinline constexpr auto& exclude_not(const collection& elements, enumeration_procedure_t<data_t, _t&> before_erase = nullptr, data_t data = data_t()) noexcept {
				return exclude<true, data_t>(elements, before_erase, data);
			}

			template<typename data_t = bool> __forceinline constexpr auto exclude_not(const collection& elements, enumeration_procedure_t<data_t, const _t&> before_erase = nullptr, data_t data = data_t()) const noexcept {
				return exclude<true, data_t>(elements, before_erase, data);
			}

			__forceinline constexpr auto& operator-=(const collection& elements) noexcept {
				return exclude(elements);
			}

			__forceinline constexpr auto operator-(const collection& elements) const noexcept {
				return exclude(elements);
			}

			__forceinline constexpr auto& push_front(const _t& element, count_t limit = -1) noexcept {
				if (count() < limit) {
					base_t::insert(base_t::begin(), element);
				}

				return *this;
			}

			__forceinline constexpr auto& push_back(const _t& element, count_t limit = -1) noexcept {
				if (count() < limit) {
					base_t::insert(base_t::end(), element); //crash here | insert -> emplace -> _Emplace_reallocate -> allocate -> new -> _malloc_base -> ntdll.dll ... | 0xC00000FD or something else; it happens when vector stored in stack
				}

				return *this;
			}

			__forceinline constexpr auto& append(const _t& element) noexcept {
				return push_back(element);
			}

			__forceinline constexpr auto append(const _t& element) const noexcept {
				auto result = *this;
				return result.append(element);
			}

			__forceinline constexpr auto& operator+=(const _t& element) noexcept {
				return append(element);
			}

			__forceinline constexpr auto operator+(const _t& element) const noexcept {
				return append(element);
			}

			__forceinline constexpr auto& append(const collection& elements) noexcept {
				if (!elements.empty()) {
					base_t::insert(base_t::end(), elements.begin(), elements.end());
				}

				return *this;
			}

			__forceinline constexpr auto append(const collection& elements) const noexcept {
				auto result = *this;
				return result.append(elements);
			}

			__forceinline constexpr auto& operator+=(const collection& elements) noexcept {
				return append(elements);
			}

			__forceinline constexpr auto operator+(const collection& elements) const noexcept {
				return append(elements);
			}

			__forceinline constexpr auto contains(const _t& value, comparison_procedure_t comparator = nullptr) const noexcept {
				if (!comparator) {
					comparator = [](const _t& l, const _t& r) {
						return int(l == r);
					};

				}

				for (const auto& element : *this) {
					if (comparator(value, element)) return true;
				}

				return false;
			}

			__forceinline constexpr operator bool() const noexcept {
				return not base_t::empty();
			}

			__forceinline constexpr void set(const collection& second) noexcept {
				memcpy(this, &second, sizeof(collection));
			}
		};

		template<typename _t, count_t _capacity> class static_collection {
		public:
			enum : index_t { npos = -1 };

		private:
			count_t _size = { };
			_t _values[_capacity] = { };

		public:
			__forceinline constexpr static_collection() = default;

			__forceinline constexpr static_collection(const _t* first, const _t* last) noexcept {
				assign(first, last);
			}

			__forceinline constexpr static_collection(const _t* data, size_t count) noexcept {
				assign(data, count);
			}

			__forceinline constexpr static_collection(const collection<_t>& collection) noexcept {
				assign(collection.data(), collection.size());
			}

			__forceinline constexpr auto empty() const noexcept {
				return _size == null;
			}

			__forceinline constexpr auto size() const noexcept {
				return _size;
			}

			__forceinline constexpr auto count() const noexcept {
				return _size;
			}

			__forceinline constexpr auto length() const noexcept {
				return _size;
			}

			__forceinline constexpr auto capacity() const noexcept {
				return _capacity;
			}

			__forceinline constexpr const auto data() const noexcept {
				return _values;
			}

			__forceinline constexpr auto data() noexcept {
				return _values;
			}

			__forceinline constexpr const auto begin() const noexcept {
				return _values;
			}

			__forceinline constexpr auto begin() noexcept {
				return _values;
			}

			__forceinline constexpr const auto end() const noexcept {
				return _values + _size;
			}

			__forceinline constexpr auto end() noexcept {
				return _values + _size;
			}

			__forceinline constexpr const auto& first() const noexcept {
				return *_values;
			}

			__forceinline constexpr auto& first() noexcept {
				return *_values;
			}

			__forceinline constexpr const auto& front() const noexcept {
				return *_values;
			}

			__forceinline constexpr auto& front() noexcept {
				return *_values;
			}

			__forceinline constexpr const auto& last() const noexcept {
				return *_values;
			}

			__forceinline constexpr auto& last() noexcept {
				return *_values;
			}

			__forceinline constexpr const auto& back() const noexcept {
				return *_values;
			}

			__forceinline constexpr auto& back() noexcept {
				return *_values;
			}

			__forceinline constexpr void assign(const _t* first, const _t* last) noexcept {
				auto destination = _values;
				auto destination_end = _values + _capacity;

				if (!destination) return;

				auto source = first;
				auto source_end = last;

				for (_size = null; destination != destination_end; destination++) {
					auto& value = *destination;

					if (!source || source >= source_end) {
						memset(&value, null, sizeof(_t));
					}
					else {
						value = *(source++);
						_size++;
					}
				}
			}

			__forceinline constexpr void assign(const _t* data, size_t count) noexcept {
				return assign(data, data + count);
			}

			__forceinline constexpr void insert(index_t position, const _t* data, size_t count, _t** _out = nullptr, size_t* _out_count = nullptr, bool shift_right = true) {
				if (!_out_count) {
					auto buffer = size_t();
					_out_count = &buffer;
				}

				if (shift_right) {
					utils::shift<true>(_values, _capacity, _size, position, count, false, _out, _out_count);
				}
				else {
					utils::shift<false>(_values, _capacity, _size, position, count, false, _out, _out_count);
				}

				auto size = _size - *_out_count;

				auto dst = _values + position, end = dst + count;
				for (auto src = data; dst != end; src++, dst++, size++) {
					*dst = *src;
				}

				_size = size;
			}

			__forceinline constexpr void insert(index_t position, const _t* first, const _t* last, _t** _out = nullptr, size_t* _out_count = nullptr, bool shift_right = true) noexcept {
				return insert(position, first, last - first, _out, _out_count, shift_right);
			}

			__forceinline constexpr void insert(const _t* position, const _t* first, const _t* last, _t** _out = nullptr, size_t* _out_count = nullptr, bool shift_right = true) noexcept {
				return insert(position - _values, first, last - first, _out, _out_count, shift_right);
			}

			__forceinline constexpr auto push_back(const _t& value, _t** _out = nullptr, size_t* _out_count = nullptr) noexcept {
				return insert(_size, &value, 1, _out, _out_count, false);
			}

			__forceinline constexpr auto push_front(const _t& value, _t** _out = nullptr, size_t* _out_count = nullptr) noexcept {
				return insert(index_t(), &value, 1, _out, _out_count, true);
			}

			__forceinline constexpr auto& append(const _t& element, _t** _out = nullptr, size_t* _out_count = nullptr) noexcept {
				return push_back(element, _out, _out_count), * this;
			}

			__forceinline constexpr auto append(const _t& element, _t** _out = nullptr, size_t* _out_count = nullptr) const noexcept {
				auto result = *this;
				return result.append(element, _out, _out_count);
			}

			__forceinline constexpr auto& operator+=(const _t& element) noexcept {
				return append(element);
			}

			__forceinline constexpr auto operator+(const _t& element) const noexcept {
				return append(element);
			}

			__forceinline constexpr auto& append(const collection<_t>& elements, _t** _out = nullptr, size_t* _out_count = nullptr) noexcept {
				return insert(end(), elements.data(), elements.data() + elements.size(), _out, _out_count), * this;
			}

			__forceinline constexpr auto append(const collection<_t>& elements, _t** _out = nullptr, size_t* _out_count = nullptr) const noexcept {
				auto result = *this;
				return result.append(elements, _out, _out_count);
			}

			__forceinline constexpr auto& operator+=(const collection<_t>& elements) noexcept {
				return append(elements);
			}

			__forceinline constexpr auto operator+(const collection<_t>& elements) const noexcept {
				return append(elements);
			}

			__forceinline constexpr auto erase(index_t position) noexcept {
				if (position > _size) return;

				auto begin = _values;
				auto end = begin + _size;

				auto destination = begin + position;
				auto source = destination + 1;

				(*destination).~_t();
				memset(destination, null, sizeof(_t));

				for (_size--; source != end; source++, destination++) {
					*destination = *source;
				}
			}

			__forceinline constexpr auto erase(_t* position) noexcept {
				return erase((position - _values) / sizeof(_t));
			}

			__forceinline constexpr auto& exclude(_t* position) noexcept {
				return erase(position), * this;
			}

			__forceinline constexpr auto exclude(_t* position) const noexcept {
				auto result = *this;
				return result.exclude(position);
			}

			__forceinline constexpr auto& exclude(index_t position) noexcept {
				return erase(position), * this;
			}

			__forceinline constexpr auto exclude(index_t position) const noexcept {
				auto result = *this;
				return result.exclude(position);
			}

			template<bool _not = false, typename data_t = bool> __forceinline constexpr auto& exclude(const _t* elements, count_t count, collection<_t>::enumeration_procedure_t<data_t, _t&> before_erase = nullptr, data_t data = data_t()) noexcept {
				auto base = begin();
				for (index_t i = 0; i < size(); i++) {
					auto current = base + i;
					auto& element = *current;

					auto in = false;
					for (auto item = elements, end = elements + count; item != end; item++) {
						if (in = (element == *item)) break;
					}

					if constexpr (_not) {
						if (in) continue;
					}
					else if (!in) continue;

					if (before_erase) {
						if (before_erase(i, element, data) == enumeration::return_t::skip) continue;
					}

					erase(current);
					i--;
				}

				return *this;
			}

			template<bool _not = false, typename data_t = bool> __forceinline constexpr auto& exclude(const collection<_t>& elements, collection<_t>::enumeration_procedure_t<data_t, _t&> before_erase = nullptr, data_t data = data_t()) noexcept {
				return exclude<_not, data_t>(elements.data(), elements.count(), before_erase, data);
			}

			template<bool _not = false, typename data_t = bool> __forceinline constexpr auto exclude(const collection<_t>& elements, collection<_t>::enumeration_procedure_t<data_t, const _t&> before_erase = nullptr, data_t data = data_t()) const noexcept {
				auto result = *this;
				return result.exclude<_not, data_t>(elements, before_erase, data);
			}

			template<typename data_t = bool> __forceinline constexpr auto& exclude_not(const collection<_t>& elements, collection<_t>::enumeration_procedure_t<data_t, _t&> before_erase = nullptr, data_t data = data_t()) noexcept {
				return exclude<true, data_t>(elements, before_erase, data);
			}

			template<typename data_t = bool> __forceinline constexpr auto exclude_not(const collection<_t>& elements, collection<_t>::enumeration_procedure_t<data_t, const _t&> before_erase = nullptr, data_t data = data_t()) const noexcept {
				return exclude<true, data_t>(elements, before_erase, data);
			}

			__forceinline constexpr auto& operator-=(const collection<_t>& elements) noexcept {
				return exclude(elements);
			}

			__forceinline constexpr auto operator-(const collection<_t>& elements) const noexcept {
				return exclude(elements);
			}

			template<bool _full = false> __forceinline constexpr auto clear() noexcept {
				for (auto& element : *this) {
					element.~_t();
				}

				_size = null;

				if constexpr (_full) {
					memset(_values, null, _capacity * sizeof(_t));
				}
			}

			__forceinline constexpr auto sort(collection<_t>::comparison_procedure_t procedure) const noexcept {
				auto result = *this;

				if (procedure) {
					qsort(result.data(), result.size(), sizeof(_t), collection<_t>::native_comparison_procedure_t(procedure));
				}

				return result;
			}

			__forceinline constexpr auto& reverse() noexcept {
				return std::reverse(begin(), end()), * this;
			}

			__forceinline constexpr auto reverse() const noexcept {
				auto result = *this;
				return result.reverse();
			}

			template<bool __initialize = true> __forceinline constexpr auto resize(size_t size) noexcept {
				auto delta = size - _size;

				_size = size;

				if constexpr (!__initialize) return;

				if (delta > 0) {
					for (auto i = delta, k = size; i < k; i++) {
						_values[i] = _t();
					}
				}
			}

			__forceinline constexpr auto& at(index_t position) noexcept {
				return _values[position];
			}

			__forceinline constexpr const auto& at(index_t position) const noexcept {
				return _values[position];
			}

			__forceinline constexpr auto& operator[](index_t position) noexcept {
				return _values[position];
			}

			__forceinline constexpr const auto& operator[](index_t position) const noexcept {
				return _values[position];
			}
		};
	}

	using namespace types;
}