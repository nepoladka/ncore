#pragma once 
#include "defines.hpp"
#include "includes/zip_file.hpp"

#undef compress

namespace ncore::zip {
	begin_unaligned class compressed {
	public:
		size_t raw_length;
		size_t length;
		byte_t first_byte;

		template<typename _t = byte_t> __forceinline _t* data() {
			return (_t*)&first_byte;
		}

		static __forceinline size_t total_size(size_t size) {
			return size + sizeof(compressed) - sizeof(first_byte);
		}

		static __forceinline compressed* alloc(size_t size) {
			return (compressed*)malloc(total_size(size));
		}

		static __forceinline void release(compressed** instance) {
			if (!instance) return;

			if (!*instance) return;

			auto address = *instance;
			*instance = nullptr;

			return address->release();
		}

		__forceinline void release() {
			return free(this);
		}


		__forceinline bool decompress(void** _result, size_t* _length) {
			if (!(_result && _length)) _Fail: return false;

			auto raw_length = mz_ulong(this->raw_length);
			auto length = mz_ulong(this->length);
			if (!(length && raw_length)) goto _Fail;

			auto result = (byte_t*)malloc(raw_length);
			if (mz_uncompress(result, &raw_length, data(), length) != MZ_OK) {
				free(result);
				goto _Fail;
			}

			*_result = result;
			*_length = raw_length;

			return true;
		}

	private:
		__forceinline compressed() { return; }
		__forceinline ~compressed() { return; }
	} end_unaligned;

	static __forceinline compressed* compress(const void* data, size_t size) {
		if (!(data && size)) _Fail: return nullptr;

		auto length = mz_ulong(size);
		auto result = compressed::alloc(size);

		if (mz_compress2(result->data(), &length, (const byte_t*)data, length, MZ_UBER_COMPRESSION) != MZ_OK) {
			result->release();
			goto _Fail;
		}

		result->raw_length = size;
		result->length = compressed::total_size(length);

		return (compressed*)realloc(result, result->length);
	}

	static __forceinline bool decompress(compressed* data, void** _result, size_t* _length) {
		return data ? data->decompress(_result, _length) : false;
	}
}
