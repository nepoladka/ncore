#pragma once

namespace ncore {
	using r8_t = unsigned char;

	union r16_t {
		struct {
			r8_t primary, secondary;
		}value;

		unsigned short summary;
	};

	union r32_t {
		struct {
			r16_t primary, secondary;
		}value;

		unsigned long summary;
	};

	union r64_t {
		struct {
			r32_t primary, secondary;
		}value;

		unsigned long long summary;
	};

	enum class result : r8_t{
		failure,
		success,

		count
	};
}
