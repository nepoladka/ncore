#pragma once
#include "unhashed_map.hpp"

#include <string>

#ifndef NCORE_TAGGED_POOL_TAKE
#define NCORE_TAGGED_POOL_TAKE(SIZE) ::malloc(SIZE)
#endif

#ifndef NCORE_TAGGED_POOL_RELEASE
#define NCORE_TAGGED_POOL_RELEASE(TARGET, SIZE) ::free(TARGET), true
#endif

namespace ncore {
	template<typename tag_t = std::string, typename address_t = ncore::address_t> class tagged_pool {
	public:
		struct allocation_info {
			address_t address;
			size_t size;
		};

	private:
		unhashed_map<tag_t, allocation_info> _allocations;

	public:
		__forceinline constexpr tagged_pool() = default;
		__forceinline constexpr ~tagged_pool() = default;

		__forceinline constexpr allocation_info set(tag_t tag, address_t address, size_t size) noexcept {
			auto previous = pair<tag_t, allocation_info>();

			auto& target = _allocations.find_or_place(tag);
			if (target.key() == previous.key()) {
				_allocations.push_back({ tag, {address, size} });
			}
			else {
				previous = target;

				target.value() = { address, size };
			}

			return previous.value();
		}

		__forceinline constexpr address_t get(tag_t tag) const noexcept {
			return _allocations.find(tag).value().address;
		}

		template<typename _t = void> __forceinline constexpr _t* take(tag_t tag, size_t size) noexcept {
			auto target = _allocations.find(tag);
			if (target.key() == tag) return (_t*)target.value().address;

			auto address = address_t(NCORE_TAGGED_POOL_TAKE(size));
			if (address) {
				_allocations.push_back({ tag, {address, size} });
			}

			return (_t*)address;
		}

		__forceinline constexpr void release(bool free = true) noexcept {
			for (auto i = index_t(), l = _allocations.count(); i < l; i++) {
				auto& entry = _allocations[i];

				if (free) if (!(NCORE_TAGGED_POOL_RELEASE(entry.value().address, entry.value().size))) continue;

				_allocations.erase(_allocations.begin() + i);
			}
		}

		__forceinline constexpr void release(tag_t tag, bool free = true) noexcept {
			for (auto i = index_t(), l = _allocations.count(); i < l; i++) {
				auto& entry = _allocations[i];
				if (!(entry.key() == tag)) continue;

				if (free) if (!(NCORE_TAGGED_POOL_RELEASE(entry.value().address, entry.value().size))) break;

				_allocations.erase(_allocations.begin() + i);

				break;
			}
		}
	};
}
