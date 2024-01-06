#pragma once
#define NCORE_ENUMERATION_NO_BINARY
#include "enumeration.hpp"

#include <vector>

namespace ncore {
	namespace types {
		template<typename _t> class collection : public std::vector<_t> {
		public:
			using base_t = std::vector<_t>;

			enum : index_t { npos = -1 };

			template<typename data_t, typename element_t> using enumeration_procedure_t = get_procedure_t(ncore::enumeration::return_t, , index_t, element_t, data_t);
			using comparison_procedure_t = get_procedure_t(int, , const _t& left, const _t& right);
			using native_comparison_procedure_t = _CoreCrtNonSecureSearchSortCompareFunction;

			__forceinline constexpr collection() noexcept : base_t({ }) {
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
			}

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

			template<bool _not = false, typename data_t = bool> __forceinline constexpr auto& exclude(const collection& elements, enumeration_procedure_t<data_t, _t&> before_erase = nullptr, data_t data = data_t()) noexcept {
				auto base = base_t::begin();
				for (index_t i = 0; i < base_t::size(); i++) {
					auto current = base + i;
					auto& element = *current;

					auto in = false;
					for (auto& second : elements) {
						if (in = (element == second)) break;
					}

					if constexpr (_not) {
						if (in) continue;
					}
					else if (!in) continue;

					if (before_erase) {
						if (before_erase(i, element, data) == ncore::enumeration::return_t::skip) continue;
					}

					base_t::erase(current);
					i--;
				}

				return *this;
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

			__forceinline constexpr auto& append(const _t& element) noexcept {
				return base_t::push_back(element), *this;
			}

			__forceinline constexpr auto append(const _t element) const noexcept {
				auto result = *this;
				return result.append(element);
			}

			__forceinline constexpr auto& operator+=(const _t& element) noexcept {
				return append(element);
			}

			__forceinline constexpr auto operator+(const _t element) const noexcept {
				return append(element);
			}

			__forceinline constexpr auto& append(const collection& elements) noexcept {
				return base_t::insert(base_t::end(), elements.begin(), elements.end()), *this;
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
		};
	}

	using namespace types;
}