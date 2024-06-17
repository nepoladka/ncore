#include "includes/cpr/cpr.h"
#include "../source/web.hpp"

#pragma comment(lib, "cpr.lib")
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "Crypt32.lib")

ncore::web::request::response_t ncore::web::request::send(const method_t method, const url_t& url, const query_t& query) {
	using namespace cpr;

	auto response = Response();
	auto address = Url(url);
	auto parameters = Parameters(*((std::initializer_list<Parameter>*)(&query)));

	switch (method) {
	case method_t::m_get: response = Get(address, parameters); break;
	case method_t::m_post: response = Post(address, parameters); break;
	case method_t::m_put: response = Put(address, parameters); break;
	case method_t::m_head: response = Head(address, parameters); break;
	case method_t::m_delete: response = Delete(address, parameters); break;
	case method_t::m_options: response = Options(address, parameters); break;
	case method_t::m_patch: response = Patch(address, parameters); break;
	default: break;
	}

	return { response.status_code, response.text };
}

ncore::web::socket::result_t ncore::web::socket::client::connect(family family, type type, protocol protocol) {
	if (_ip.empty()) return { r_not_initialized };

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

	hints.ai_family = int(family);
	hints.ai_socktype = int(type);
	hints.ai_protocol = int(protocol);

	status = getaddrinfo(_ip.c_str(), _port.empty() ? nullptr : _port.c_str(), &hints, &address);
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

	setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&__defaultTimeout, sizeof(unsigned));
	setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&__defaultTimeout, sizeof(unsigned));

	goto _Exit;
}

ncore::web::socket::result_t ncore::web::socket::client::disconnect() {
	if (!_handle) return { r_not_initialized };

	closesocket(_handle);
	WSACleanup();
	_handle = NULL;

	return { r_success };
}

ncore::web::socket::result_t ncore::web::socket::client::send(const void* data, size_t length) {
	if (!_handle) return { r_not_initialized };

	auto result = ::send(_handle, (char*)data, length, 0);
	if (result > 0) return { r_success };

	return { r_failure, WSAGetLastError() };
}

ncore::web::socket::result_t ncore::web::socket::client::receive(void* _buffer, size_t size, size_t* _length) {
	if (!_handle) return { r_not_initialized };

	auto result = recv(_handle, (char*)_buffer, size, 0);
	*_length = result;

	if (result > 0) return { r_success };

	return { r_failure, WSAGetLastError() };
}

ncore::web::socket::result_t ncore::web::socket::client::set_timeout(bool for_receive, unsigned timeout) {
	if (!_handle) return { r_not_initialized };

	setsockopt(_handle, SOL_SOCKET, SO_SNDTIMEO + for_receive, (const char*)&timeout, sizeof(unsigned));

	return { r_success };
}
