#pragma once
#ifdef NCORE_WEB_STANDALONE
#include <string>
#include <vector>
#include <map>

namespace ncore {
	static __forceinline bool constexpr is_little_endian(unsigned __int64 value) noexcept {
		const auto bytes = (const unsigned __int8*)(&value);
		return *bytes == (value & 0xFFui8);
	}

	static __forceinline bool constexpr is_big_endian(unsigned __int64 value) noexcept {
		const auto bytes = (const unsigned __int8*)(&value);
		return *bytes != (value & 0xFFui8);
	}

	static __forceinline unsigned __int64 constexpr swap_endian(unsigned __int64 value) noexcept {
		return ((value & 0xFF00000000000000ui64) >> 56) |
			((value & 0x00FF000000000000ui64) >> 40) |
			((value & 0x0000FF0000000000ui64) >> 24) |
			((value & 0x000000FF00000000ui64) >> 8) |
			((value & 0x00000000FF000000ui64) << 8) |
			((value & 0x0000000000FF0000ui64) << 24) |
			((value & 0x000000000000FF00ui64) << 40) |
			((value & 0x00000000000000FFui64) << 56);
	}

	static __forceinline unsigned __int32 constexpr swap_endian(unsigned __int32 value) noexcept {
		return ((value & 0xFF000000ui32) >> 24) |
			((value & 0x00FF0000ui32) >> 8) |
			((value & 0x0000FF00ui32) << 8) |
			((value & 0x000000FFui32) << 24);
	}

	static __forceinline unsigned __int16 constexpr swap_endian(unsigned __int16 value) noexcept {
		return ((value & 0xFF00ui16) >> 8) |
			((value & 0x00FFui16) << 8);
	}

	static __forceinline unsigned __int8 constexpr swap_endian(unsigned __int8 value) noexcept {
		return value;
	}
}
#else
#include "defines.hpp"
#endif

//#pragma comment(lib, "nweb.lib")

namespace ncore::web {
	namespace status {
		using status_t = __int32;

		constexpr status_t INFO_CODE_OFFSET = 100;
		constexpr status_t HTTP_CONTINUE = 100;
		constexpr status_t HTTP_SWITCHING_PROTOCOL = 101;
		constexpr status_t HTTP_PROCESSING = 102;
		constexpr status_t HTTP_EARLY_HINTS = 103;

		constexpr status_t SUCCESS_CODE_OFFSET = 200;
		constexpr status_t HTTP_OK = 200;
		constexpr status_t HTTP_CREATED = 201;
		constexpr status_t HTTP_ACCEPTED = 202;
		constexpr status_t HTTP_NON_AUTHORITATIVE_INFORMATION = 203;
		constexpr status_t HTTP_NO_CONTENT = 204;
		constexpr status_t HTTP_RESET_CONTENT = 205;
		constexpr status_t HTTP_PARTIAL_CONTENT = 206;
		constexpr status_t HTTP_MULTI_STATUS = 207;
		constexpr status_t HTTP_ALREADY_REPORTED = 208;
		constexpr status_t HTTP_IM_USED = 226;

		constexpr status_t REDIRECT_CODE_OFFSET = 300;
		constexpr status_t HTTP_MULTIPLE_CHOICE = 300;
		constexpr status_t HTTP_MOVED_PERMANENTLY = 301;
		constexpr status_t HTTP_FOUND = 302;
		constexpr status_t HTTP_SEE_OTHER = 303;
		constexpr status_t HTTP_NOT_MODIFIED = 304;
		constexpr status_t HTTP_USE_PROXY = 305;
		constexpr status_t HTTP_UNUSED = 306;
		constexpr status_t HTTP_TEMPORARY_REDIRECT = 307;
		constexpr status_t HTTP_PERMANENT_REDIRECT = 308;

		constexpr status_t CLIENT_ERROR_CODE_OFFSET = 400;
		constexpr status_t HTTP_BAD_REQUEST = 400;
		constexpr status_t HTTP_UNAUTHORIZED = 401;
		constexpr status_t HTTP_PAYMENT_REQUIRED = 402;
		constexpr status_t HTTP_FORBIDDEN = 403;
		constexpr status_t HTTP_NOT_FOUND = 404;
		constexpr status_t HTTP_METHOD_NOT_ALLOWED = 405;
		constexpr status_t HTTP_NOT_ACCEPTABLE = 406;
		constexpr status_t HTTP_PROXY_AUTHENTICATION_REQUIRED = 407;
		constexpr status_t HTTP_REQUEST_TIMEOUT = 408;
		constexpr status_t HTTP_CONFLICT = 409;
		constexpr status_t HTTP_GONE = 410;
		constexpr status_t HTTP_LENGTH_REQUIRED = 411;
		constexpr status_t HTTP_PRECONDITION_FAILED = 412;
		constexpr status_t HTTP_PAYLOAD_TOO_LARGE = 413;
		constexpr status_t HTTP_URI_TOO_LONG = 414;
		constexpr status_t HTTP_UNSUPPORTED_MEDIA_TYPE = 415;
		constexpr status_t HTTP_REQUESTED_RANGE_NOT_SATISFIABLE = 416;
		constexpr status_t HTTP_EXPECTATION_FAILED = 417;
		constexpr status_t HTTP_IM_A_TEAPOT = 418;
		constexpr status_t HTTP_MISDIRECTED_REQUEST = 421;
		constexpr status_t HTTP_UNPROCESSABLE_ENTITY = 422;
		constexpr status_t HTTP_LOCKED = 423;
		constexpr status_t HTTP_FAILED_DEPENDENCY = 424;
		constexpr status_t HTTP_TOO_EARLY = 425;
		constexpr status_t HTTP_UPGRADE_REQUIRED = 426;
		constexpr status_t HTTP_PRECONDITION_REQUIRED = 428;
		constexpr status_t HTTP_TOO_MANY_REQUESTS = 429;
		constexpr status_t HTTP_REQUEST_HEADER_FIELDS_TOO_LARGE = 431;
		constexpr status_t HTTP_UNAVAILABLE_FOR_LEGAL_REASONS = 451;

		constexpr status_t SERVER_ERROR_CODE_OFFSET = 500;
		constexpr status_t HTTP_INTERNAL_SERVER_ERROR = 500;
		constexpr status_t HTTP_NOT_IMPLEMENTED = 501;
		constexpr status_t HTTP_BAD_GATEWAY = 502;
		constexpr status_t HTTP_SERVICE_UNAVAILABLE = 503;
		constexpr status_t HTTP_GATEWAY_TIMEOUT = 504;
		constexpr status_t HTTP_HTTP_VERSION_NOT_SUPPORTED = 505;
		constexpr status_t HTTP_VARIANT_ALSO_NEGOTIATES = 506;
		constexpr status_t HTTP_INSUFFICIENT_STORAGE = 507;
		constexpr status_t HTTP_LOOP_DETECTED = 508;
		constexpr status_t HTTP_NOT_EXTENDED = 510;
		constexpr status_t HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511;
		constexpr status_t MISC_CODE_OFFSET = 600;

		static __forceinline constexpr bool const is_informational(const status_t code) noexcept {
			return (code >= INFO_CODE_OFFSET && code < SUCCESS_CODE_OFFSET);
		}

		static __forceinline constexpr bool const is_success(const status_t code) noexcept {
			return (code >= SUCCESS_CODE_OFFSET && code < REDIRECT_CODE_OFFSET);
		}

		static __forceinline constexpr bool const is_redirect(const status_t code) noexcept {
			return (code >= REDIRECT_CODE_OFFSET && code < CLIENT_ERROR_CODE_OFFSET);
		}

		static __forceinline constexpr bool const is_client_error(const status_t code) noexcept {
			return (code >= CLIENT_ERROR_CODE_OFFSET && code < SERVER_ERROR_CODE_OFFSET);
		}

		static __forceinline constexpr bool const is_server_error(const status_t code) noexcept {
			return (code >= SERVER_ERROR_CODE_OFFSET && code < MISC_CODE_OFFSET);
		}
	}

	namespace request {
		enum method_t : __int8 {
			m_get,
			m_post,
			m_put,
			m_head,
			m_delete,
			m_options,
			m_patch,

			m_count
		};

		struct parameter_t {
			__forceinline parameter_t(const std::string& p_key, const std::string& p_value) : key{ p_key }, value{ p_value } { return; }
			__forceinline parameter_t(std::string&& p_key, std::string&& p_value) : key{ std::move(p_key) }, value{ std::move(p_value) } { return; }

			std::string key, value;
		};

		struct response_t {
			status::status_t code;
			std::string data;

			__forceinline constexpr bool const is_informational() const noexcept {
				return status::is_informational(code);
			}

			__forceinline constexpr bool const is_success() const noexcept {
				return status::is_success(code);
			}

			__forceinline constexpr bool const is_redirect() const noexcept {
				return status::is_redirect(code);
			}

			__forceinline constexpr bool const is_client_error() const noexcept {
				return status::is_client_error(code);
			}

			__forceinline constexpr bool const is_server_error() const noexcept {
				return status::is_server_error(code);
			}
		};

		using url_t = std::string;
		using query_t = std::initializer_list<parameter_t>;
		using body_t = std::vector<unsigned __int8>;
		using headers_t = std::initializer_list<std::pair<std::string, std::string>>;

		response_t send(const method_t method, const url_t& url, const query_t& parameters = { }, const headers_t& headers = { }, const body_t & body = { });
	}

	namespace socket {
		static constexpr const unsigned const __defaultTimeout = 1000 * 60 * 25; //25mins

		enum result : __int8 {
			r_success,
			r_failure,
			r_not_initialized,
			r_wsa_startup_failed,
			r_get_address_info_failed,
			r_socket_failed,
		};

		enum family : __int32 {
			f_unspec = 0,           // unspecified
			f_unix = 1,           // local to host (pipes, portals)
			f_inet = 2,           // internetwork: UDP, TCP, etc.
			f_implink = 3,             // arpanet imp addresses
			f_pup = 4,          // pup protocols: e.g. BSP
			f_chaos = 5,          // mit CHAOS protocols
			f_ns = 6,        // XEROX NS protocols
			f_ipx = f_ns,        // IPX protocols: IPX, SPX, etc.
			f_iso = 7,       // ISO protocols
			f_osi = f_iso,       // OSI is ISO
			f_ecma = 8,          // european computer manufacturers
			f_datakit = 9,            // datakit protocols
			f_ccitt = 10,          // CCITT protocols, X.25 etc
			f_sna = 11,           // IBM SNA
			f_decnet = 12,         // DECnet
			f_dli = 13,        // Direct data link interface
			f_lat = 14,         // LAT
			f_hylink = 15,            // NSC Hyperchannel
			f_appletalk = 16,           // AppleTalk
			f_netbios = 17,         // NetBios-style addresses
			f_voiceview = 18,           // VoiceView
			f_firefox = 19,         // Protocols from Firefox
			f_unknown1 = 20,         // Somebody is using this!
			f_ban = 21,           // Banyan
			f_atm = 22,             // Native ATM Services
			f_inet6 = 23,            // Internetwork Version 6
			f_cluster = 24,           // Microsoft Wolfpack
			f_12844 = 25,            // IEEE 1284.4 WG AF
			f_irda = 26,             // IrDA
			f_netdes = 28,              // Network Designers OSI & gateway

#if(_WIN32_WINNT < 0x0501)
			f_max = 29,
#else //(_WIN32_WINNT < 0x0501)

			f_tcnprocess = 29,
			f_tcnmessage = 30,
			f_iclfxbm = 31,

#if(_WIN32_WINNT < 0x0600)
			f_max = 32,
#else //(_WIN32_WINNT < 0x0600)
			f_bth = 32,              // Bluetooth RFCOMM/L2CAP protocols
#if(_WIN32_WINNT < 0x0601)
			f_max = 33,
#else //(_WIN32_WINNT < 0x0601)
			f_link = 33,
#if(_WIN32_WINNT < 0x0604)
			f_MAX = 34,
#else //(_WIN32_WINNT < 0x0604)
			f_hyperv = 34,
			f_max = 35,
#endif //(_WIN32_WINNT < 0x0604)
#endif //(_WIN32_WINNT < 0x0601)
#endif //(_WIN32_WINNT < 0x0600)

#endif //(_WIN32_WINNT < 0x0501)
		};

		enum type : __int32 {
			t_stream = 1,               /* stream socket */
			t_dgram = 2,               /* datagram socket */
			t_raw = 3,               /* raw-protocol interface */
			t_rdm = 4,               /* reliably-delivered message */
			t_seqpacket = 5,               /* sequenced packet stream */
		};

		enum protocol : __int32 {
#if(_WIN32_WINNT >= 0x0501)
			p_hopopts = 0,  // IPv6 Hop-by-Hop options
#endif//(_WIN32_WINNT >= 0x0501)
			p_icmp = 1,
			p_igmp = 2,
			p_ggp = 3,
#if(_WIN32_WINNT >= 0x0501)
			p_ipv4 = 4,
#endif//(_WIN32_WINNT >= 0x0501)
#if(_WIN32_WINNT >= 0x0600)
			p_st = 5,
#endif//(_WIN32_WINNT >= 0x0600)
			p_tcp = 6,
#if(_WIN32_WINNT >= 0x0600)
			p_cbt = 7,
			p_egp = 8,
			p_igp = 9,
#endif//(_WIN32_WINNT >= 0x0600)
			p_pup = 12,
			p_udp = 17,
			p_idp = 22,
#if(_WIN32_WINNT >= 0x0600)
			p_rdp = 27,
#endif//(_WIN32_WINNT >= 0x0600)

#if(_WIN32_WINNT >= 0x0501)
			p_ipv6 = 41, // IPv6 header
			p_routing = 43, // IPv6 Routing header
			p_fragment = 44, // IPv6 fragmentation header
			p_esp = 50, // encapsulating security payload
			p_ah = 51, // authentication header
			p_icmpv6 = 58, // ICMPv6
			p_none = 59, // IPv6 no next header
			p_dstopts = 60, // IPv6 Destination options
#endif//(_WIN32_WINNT >= 0x0501)

			p_nd = 77,
#if(_WIN32_WINNT >= 0x0501)
			p_iclfxbm = 78,
#endif//(_WIN32_WINNT >= 0x0501)
#if(_WIN32_WINNT >= 0x0600)
			p_pim = 103,
			p_pgm = 113,
			p_l2tp = 115,
			p_sctp = 132,
#endif//(_WIN32_WINNT >= 0x0600)
			p_raw = 255,

			p_max = 256,
			//
			//  These are reserved for internal use by Windows.
			//
			p_reserved_raw = 257,
			p_reserved_ipsec = 258,
			p_reserved_ipsecoffload = 259,
			p_reserved_wnv = 260,
			p_reserved_max = 261,
		};

		struct result_t {
			result index;
			__int32 status;

			__forceinline constexpr result_t(result index = { }, __int32 status = { }) noexcept :
				index(index),
				status(status) {
				return;
			}

			__forceinline constexpr bool const is_success() const noexcept {
				return index == result::r_success;
			}

			__forceinline constexpr bool const is_error() const noexcept {
				return index != result::r_success;
			}
		};

		class client {
		private:
			using handle_t = unsigned __int64;

			std::string _ip, _port;
			handle_t _handle;

		public:
			__forceinline constexpr client(const std::string& ip, const std::string& port) noexcept :
				_ip(ip),
				_port(port),
				_handle(handle_t()) {
				return;
			}

			result_t connect(family family = family::f_unspec, type type = t_stream, protocol protocol = protocol::p_tcp);
			result_t disconnect();
			result_t send(const void* data, size_t length);
			result_t receive(void* _buffer, size_t size, size_t* _length);

			result_t set_timeout(bool for_receive, unsigned timeout);
		};
	}
}
