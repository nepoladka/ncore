#pragma once
#include <string>

//#pragma comment(lib, "ncore_web.lib")

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

		constexpr bool is_informational(const status_t code) noexcept {
			return (code >= INFO_CODE_OFFSET && code < SUCCESS_CODE_OFFSET);
		}

		constexpr bool is_success(const status_t code) noexcept {
			return (code >= SUCCESS_CODE_OFFSET && code < REDIRECT_CODE_OFFSET);
		}

		constexpr bool is_redirect(const status_t code) noexcept {
			return (code >= REDIRECT_CODE_OFFSET && code < CLIENT_ERROR_CODE_OFFSET);
		}

		constexpr bool is_client_error(const status_t code) noexcept {
			return (code >= CLIENT_ERROR_CODE_OFFSET && code < SERVER_ERROR_CODE_OFFSET);
		}

		constexpr bool is_server_error(const status_t code) noexcept {
			return (code >= SERVER_ERROR_CODE_OFFSET && code < MISC_CODE_OFFSET);
		}
	}

	namespace request {
		struct parameter_t {
			__forceinline parameter_t(const std::string& p_key, const std::string& p_value) : key{ p_key }, value{ p_value } { return; }
			__forceinline parameter_t(std::string&& p_key, std::string&& p_value) : key{ std::move(p_key) }, value{ std::move(p_value) } { return; }

			std::string key, value;
		};

		using query_t = std::initializer_list<parameter_t>;

		struct response_t {
			status::status_t code;
			std::string data;
		};

		enum method : __int8 {
			m_get,
			m_post,
			m_put,
			m_head,
			m_delete,
			m_options,
			m_patch,

			m_count
		};

		using method_t = method;

		response_t send(const method_t method, const std::string& url, const query_t& parameters);
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

		struct result_t {
			result index = r_success;
			__int32 status = 0;
		};

		class client {
		private:
			using handle_t = unsigned __int64;

			std::string _ip, _port;
			handle_t _handle;

		public:
			__forceinline client(const std::string& ip, const std::string& port) {
				_ip = ip;
				_port = port;
				_handle = 0;
			}

			result_t connect();
			bool disconnect();
			bool send(const void* data, size_t length);
			bool receive(void* _buffer, size_t size, size_t* _length);

			bool set_timeout(bool for_receive, unsigned timeout);
		};
	}
}
