#pragma once
#include "collection.hpp"
#include "pair.hpp"

namespace ncore {
	template<typename key_t, typename value_t> class unhashed_map : public collection<pair<key_t, value_t>> {
	public:
		using entry_t = pair<key_t, value_t>;
		using base_t = collection<entry_t>;

	public:
		__forceinline constexpr unhashed_map() noexcept : base_t({ }) {
			return;
		}

		__forceinline constexpr unhashed_map(std::initializer_list<entry_t> list) noexcept : base_t(list) {
			return;
		}

		__forceinline constexpr unhashed_map(const entry_t* data, const count_t count) noexcept : base_t(data, data + count) {
			return;
		}

		__forceinline constexpr unhashed_map(entry_t* data, count_t count) noexcept : base_t(data, data + count) {
			return;
		}

		__forceinline constexpr unhashed_map(count_t count, const entry_t& value = entry_t()) noexcept : base_t(count, value) {
			return;
		}

		__forceinline constexpr auto contains(const key_t& key) const noexcept {
			return this->contains({ key }, [](const entry_t& l, const entry_t& r) { return int(l.key() == r.key()); });
		}

		__forceinline constexpr auto& get(const key_t& key) noexcept {
			for (auto& entry : *this) {
				if (entry.key() == key) return entry;
			}

			this->push_back({ key, value_t() });

			return collection<entry_t>::back(); // this->back();
		}

		__forceinline constexpr const auto& find(const key_t& key, const entry_t& no_key_value = { }) const noexcept {
			for (auto& entry : *this) {
				if (entry.key() == key) return entry;
			}

			return no_key_value;
		}

		__forceinline constexpr const auto& operator[](index_t index) const noexcept {
			return collection<entry_t>::operator[](index);
		}

		__forceinline constexpr auto& operator[](const key_t& key) noexcept {
			return get(key).value();
		}
	};
}
