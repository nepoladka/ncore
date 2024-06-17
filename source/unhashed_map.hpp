#pragma once
#include "collection.hpp"
#include "pair.hpp"

namespace ncore {
	template<typename key_t, typename value_t> class unhashed_map : public collection<pair<key_t, value_t>> {
	public:
		using entry_t = pair<key_t, value_t>;

	public:
		__forceinline constexpr auto contains(const key_t& key) const noexcept {
			return this->contains({ key }, [](const entry_t& l, const entry_t& r) { return int(l.key() == r.key()); });
		}

		__forceinline constexpr auto& find(const key_t& key, value_t no_key_value = { }) const noexcept {
			for (auto& entry : *this) {
				if (entry.key() == key) return entry;
			}

			return no_key_value;
		}
	};
}
