#pragma once
#include "unhashed_map.hpp"

#include <string>

#ifndef NCORE_TAGGED_POOL_TAKE
#define NCORE_TAGGED_POOL_TAKE(SIZE) malloc(SIZE)
#endif

#ifndef NCORE_TAGGED_POOL_RELEASE
#define NCORE_TAGGED_POOL_RELEASE(TARGET) free(TARGET), true
#endif

namespace ncore {
	template<typename tag_t = std::string, typename address_t = ncore::address_t> class tagged_pool {
	public:
		struct allocation_info {
			address_t address;
			size_t size;
		};

	private:
		unhashed_map<tag_t, address_t> _allocations;

	public:
		__forceinline constexpr tagged_pool() = default;

		__forceinline constexpr ~tagged_pool() {
			for (auto& entry : _allocations) {
				release(entry.value());
			}
		}

		__forceinline constexpr tagged_pool(const tagged_pool&) = delete;
		__forceinline constexpr tagged_pool& operator=(const tagged_pool&) = delete;

		__forceinline constexpr allocation_info set(tag_t tag, address_t address, size_t size) noexcept {
			auto previous = allocation_info();

			auto& target = _allocations.find(tag, previous);
			if (target.key() == previous.key()) {
				_allocations.push_back({ tag, {address, size} });
			}
			else {
				previous = target;

				target.value() = { address, size };
			}
			
			return previous;
		}

		__forceinline constexpr address_t get(tag_t tag) const noexcept {
			return _allocations.find(tag).value();
		}

		__forceinline constexpr address_t take(tag_t tag, size_t size) noexcept {
			auto target = _allocations.find(tag);
			if (target.key() == tag) return target.value();

			auto address = address_t(NCORE_TAGGED_POOL_TAKE(size));
			if (address) {
				_allocations.push_back({ tag, {address, size} });
			}

			return address;
		}

		__forceinline constexpr bool release(tag_t tag) noexcept {
			for (auto i = index_t(), l = _allocations.count(); i < l; i++) {
				auto entry = _allocations[i];
				if (!(entry.key() == tag)) continue;

				if (NCORE_TAGGED_POOL_RELEASE(entry.value())) return _allocations.erase(_allocations.begin() + i), true;

				break;
			}

			return false;
		}
	};
}
