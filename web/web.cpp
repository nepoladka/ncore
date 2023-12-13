#include "includes/cpr/cpr.h"
#include "../source/web.hpp"

#pragma comment(lib, "cpr.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")

ncore::web::request::response_t ncore::web::request::send(const method_t method, const std::string& url, const query_t& parameters) {
	using namespace cpr;

	static auto wrapper = [](const method_t method, const std::string& url, Parameters parameters) {
		switch (method) {
		case method_t::m_get: return Get(Url{ url }, parameters);
		case method_t::m_post: return Post(Url{ url }, parameters);
		case method_t::m_put: return Put(Url{ url }, parameters);
		case method_t::m_head: return Head(Url{ url }, parameters);
		case method_t::m_delete: return Delete(Url{ url }, parameters);
		case method_t::m_options: return Options(Url{ url }, parameters);
		case method_t::m_patch: return Patch(Url{ url }, parameters);
		default: break;
		}
		return Response();
	};

	auto response = wrapper(method, url, Parameters{ (*(std::initializer_list<Parameter>*)(&parameters)) });

	return { response.status_code, response.text };
}

ncore::web::socket::result_t ncore::web::socket::client::connect() {
	if (_ip.empty() || _port.empty()) return { r_not_initialized };

	struct addrinfo* address = NULL, hints = { NULL };

	auto socket = INVALID_SOCKET;
	auto wsa = WSADATA{ NULL };
	auto result = result_t{ r_success, NO_ERROR };

	auto status = WSAStartup(MAKEWORD(2, 2), &wsa);
	if (status != NO_ERROR) {
		result = { r_wsa_startup_failed, status };

	_Exit:
		return result;
	}

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	status = getaddrinfo(_ip.c_str(), _port.c_str(), &hints, &address);
	if (status != NO_ERROR) {
		result = { r_get_address_info_failed, status };

	_CleanupAndExit:
		WSACleanup();

		goto _Exit;
	}

	for (auto p = address; p != NULL; p = p->ai_next) {
		if ((socket = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == INVALID_SOCKET) {
			result = { r_socket_failed, status = WSAGetLastError() };

			goto _CleanupAndExit;
		}

		if ((status = ::connect(socket, p->ai_addr, p->ai_addrlen)) == SOCKET_ERROR) {
			closesocket(socket);
			socket = INVALID_SOCKET;
			continue;
		}

		break;
	}

	freeaddrinfo(address);

	if (socket == INVALID_SOCKET) {
		result = { r_failure };

		goto _CleanupAndExit;
	}

	_handle = socket;

	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&__defaultTimeout, sizeof(unsigned));
	setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&__defaultTimeout, sizeof(unsigned));

	goto _Exit;
}

bool ncore::web::socket::client::disconnect() {
	if (!_handle) return false;

	closesocket(_handle);
	WSACleanup();
	_handle = NULL;

	return true;
}

bool ncore::web::socket::client::send(const void* data, size_t length) {
	if (!_handle) return false;

	return ::send(_handle, (char*)data, length, 0) > 0;
}

bool ncore::web::socket::client::receive(void* _buffer, size_t size, size_t* _length) {
	if (!_handle) return false;

	return (*_length = recv(_handle, (char*)_buffer, size, 0)) > 0;
}

bool ncore::web::socket::client::set_timeout(bool for_receive, unsigned timeout) {
	if (!_handle) return false;

	setsockopt(_handle, SOL_SOCKET, SO_SNDTIMEO + for_receive, (const char*)&timeout, sizeof(unsigned));

	return true;
}
