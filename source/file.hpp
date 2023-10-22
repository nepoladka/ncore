#pragma once
#include "aligned.hpp"
#include "handle.hpp"
#include "defines.hpp"
#include "strings.hpp"
#include "includes/ntos.h"
#include <windows.h>
#include <shlobj.h>
#include <vector>
#include <filesystem>

namespace ncore {
	static const handle::native_handle_t::closer_t const __fileHandleCloser = handle::native_handle_t::closer_t(NtClose);
	static constexpr const unsigned const __defaultFileOpenAccess = FILE_ALL_ACCESS;
	static constexpr const unsigned const __defaultFileShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE;
	static constexpr const lsize_t const __fileReadWriteLimit = 1024 * 1024 * 1024 * 2; //2gb
	static constexpr const size_t const __filePathLimit = MAX_PATH;

	class file {
	public:
		enum class create_disposion : ui32_t {
			create_force = FILE_SUPERSEDE,
			create_new = FILE_CREATE,
			open_existing = FILE_OPEN,
			open_or_create = FILE_OPEN_IF,
			overwrite_existing = FILE_OVERWRITE,
			overwrite_or_create = FILE_OVERWRITE_IF
		};

		using ustring_t = UNICODE_STRING;
		using status_t = IO_STATUS_BLOCK;

		struct path_t : private ustring_t {
		private:
			bool _release;

			__forceinline void take(const std::wstring& path) {
				if (path.empty()) return;

				_release = RtlDosPathNameToNtPathName_U(path.c_str(), this, null, null);
			}

			__forceinline void take(const std::string& path) {
				if (path.empty()) return;

				auto path_length = path.length() + 1;
				auto path_buffer = (wchar_t*)malloc(path_length);
				u8tou16(path.c_str(), lsize_t(path_length), path_buffer, lsize_t(path_length));

				take(path_buffer);

				return free(path_buffer);
			}

			__forceinline auto release() noexcept {
				if (!_release) return;

				if (Buffer) {
					RtlFreeUnicodeString(this);
				}

				_release = false;
			}

		public:
			__forceinline constexpr path_t() = default;

			__forceinline path_t(const std::wstring& path) noexcept {
				take(path);
			}

			__forceinline path_t(const wchar_t* path) noexcept {
				if (path) {
					take(path);
				}
			}

			__forceinline path_t(const std::string& path) noexcept {
				take(path);
			}

			__forceinline path_t(const char* path) noexcept {
				if (path) {
					take(path);
				}
			}

			__forceinline ~path_t() noexcept {
				release();
			}

			__forceinline auto wstring() const noexcept {
				auto result = std::wstring();
				if (Buffer) {
					result = Buffer;
				}
				return result;
			}

			__forceinline auto string() const noexcept {
				auto result = std::string();
				if (Buffer) {
					auto wstring = std::wstring(Buffer);
					result.resize(wstring.length());

					u16tou8(wstring.c_str(), lsize_t(wstring.length()), result.data(), lsize_t(result.length()));
				}
				return result;
			}

			__forceinline constexpr auto empty() const noexcept {
				return !Buffer || !Length;
			}

			__forceinline auto& operator=(const path_t& path) noexcept {
				Length = path.Length;
				MaximumLength = path.MaximumLength;
				Buffer = path.Buffer;

				_release = path._release;
				((path_t*)&path)->_release = false;

				return *this;
			}
		};

		static __forceinline handle::native_t __fastcall get_handle(const path_t& path, ui32_t open_access = __defaultFileOpenAccess, create_disposion disposion = create_disposion::open_existing, ui32_t share_access = __defaultFileShareAccess, ui32_t create_options = null, status_t* _status = nullptr) noexcept {
			static constexpr const auto __defaultFileAttributes = ui32_t(FILE_ATTRIBUTE_NORMAL);
			static constexpr const auto __defaultCreateOptions = ui32_t(FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);

			auto result = handle::native_t();
			auto attributes = OBJECT_ATTRIBUTES();

			if (!_status) {
				auto status = status_t();
				_status = &status;
			}

			InitializeObjectAttributes(&attributes, PUNICODE_STRING(&path), OBJ_CASE_INSENSITIVE, NULL, NULL);

			return NT_SUCCESS(NtCreateFile(&result, open_access, &attributes,
				_status, null, __defaultFileAttributes, share_access, ui32_t(disposion),
				create_options ? __defaultCreateOptions | create_options : __defaultCreateOptions, null, null)) ?
				result : nullptr;
		}

		static __forceinline auto get_path(handle::native_t handle) noexcept {
			auto result = path_t();

			if (!handle) _Exit: return result;

			auto status = status_t();

			auto size = lsize_t(sizeof(FILE_NAME_INFORMATION::FileNameLength) + __filePathLimit * sizeof(wchar_t));
			auto info = PFILE_NAME_INFORMATION(memset(malloc(size), null, size));

			if (NT_SUCCESS(NtQueryInformationFile(handle, &status, info, size, FILE_INFORMATION_CLASS::FileNameInformation))) {
				result = path_t(info->FileName);
			}

			free(info);

			goto _Exit;
		}

	private:
		using handle_t = handle::native_handle_t;

		path_t _path;

		static __forceinline auto __fastcall temp_handle(const path_t& path, const handle_t& source, ui32_t open_access, create_disposion disposion, ui32_t share_access = __defaultFileShareAccess, ui32_t create_options = null, status_t* _status = nullptr) noexcept {
			auto result = source;
			if (!result) {
				(result = handle_t(get_handle(path, open_access, disposion, share_access, create_options, _status), __fileHandleCloser, false)).close_on_destroy(true);
			}
			return result;
		}

		__forceinline size_t get_size(handle::native_t handle) const noexcept {
			if (!handle) return null;

			auto status = status_t();
			auto info = FILE_STANDARD_INFORMATION();
			NtQueryInformationFile(handle, &status, &info, sizeof(info), FILE_INFORMATION_CLASS::FileStandardInformation);

			return *size_p(&info.EndOfFile);
		}

	public:
		__forceinline __fastcall file(const path_t& path = path_t()) noexcept {
			_path = path;
		}

		__forceinline __fastcall file(const std::string& path = std::string()) noexcept {
			_path = path;
		}

		__forceinline __fastcall file(const char* path = nullptr) noexcept {
			_path = path;
		}

		__forceinline __fastcall file(const std::wstring& path = std::wstring()) noexcept {
			_path = path;
		}

		__forceinline __fastcall file(const wchar_t* path = nullptr) noexcept {
			_path = path;
		}

		__forceinline __fastcall file(handle::native_t handle) noexcept {
			_path = get_path(handle);
		}

		static __forceinline file open(const path_t& path) noexcept {
			return file(path);
		}

		static __forceinline file create(const path_t& path) noexcept {
			temp_handle(path, handle_t(), SYNCHRONIZE, create_disposion::create_force);

			return file(path);
		}

		__forceinline std::string path() const noexcept {
			return _path.string();
		}

		__forceinline std::wstring wpath() const noexcept {
			return _path.wstring();
		}

		__forceinline bool exists() const noexcept {
			return temp_handle(_path, handle_t(), SYNCHRONIZE, create_disposion::open_existing).get();
		}

		__forceinline bool erase() noexcept {
			auto handle = temp_handle(_path, handle_t(), DELETE | SYNCHRONIZE, create_disposion::open_existing, FILE_SHARE_DELETE, FILE_DELETE_ON_CLOSE);
			if (handle.get()) {
				handle.close();

				return !exists();
			}

			return false;
		}

		__forceinline auto get_size() const noexcept {
			return get_size(temp_handle(_path, handle_t(), SYNCHRONIZE, create_disposion::open_existing).get());
		}

		__forceinline bool write_memory(offset_t offset, size_t size, const void* data, create_disposion disposion = create_disposion::open_or_create) const noexcept {
			if (!data || !size) return false;

			auto handle = temp_handle(_path, handle_t(), FILE_WRITE_DATA | SYNCHRONIZE, disposion, FILE_SHARE_READ);
			if (!handle) return false;

			if (offset > get_size(handle.get())) return false;

			static constexpr const auto write = [](const handle::native_t handle, const offset_t offset, const lsize_t size, const byte_p buffer) noexcept {
				auto status = status_t();
				return NtWriteFile(handle, nullptr, nullptr, nullptr, &status, buffer, size, PLARGE_INTEGER(&offset), nullptr);
			};

			if (size > __fileReadWriteLimit) {
				auto parts = aligned<size_t>(size, __fileReadWriteLimit);
				for (size_t i = 0; i < parts.count; i++) {
					const auto current_offset = parts.divided * i;
					write(handle.get(), offset + current_offset, parts.divided, byte_p(data) + current_offset);
				}

				if (parts.low) {
					const auto current_offset = parts.divided * parts.count;
					write(handle.get(), offset + current_offset, parts.divided, byte_p(data) + current_offset);
				}
			}
			else {
				write(handle.get(), offset, size, byte_p(data));
			}

			return true;
		}

		template<typename _t> auto write_memory(offset_t offset, const _t& data) const noexcept {
			return write_memory(offset, sizeof(_t), &data);
		}

		__forceinline auto write(const void* data, size_t size) const noexcept {
			return write_memory(null, size, data, create_disposion::create_force);
		}

		__forceinline auto write(const std::vector<byte_t>& data) noexcept {
			return write_memory(null, data.size(), data.data(), create_disposion::create_force);
		}

		__forceinline bool read_memory(offset_t offset, size_t size, void* _data) const noexcept {
			if (!_data) return false;

			auto handle = temp_handle(_path, handle_t(), FILE_READ_DATA | SYNCHRONIZE, create_disposion::open_existing, __defaultFileShareAccess);
			if (!handle) return false;

			auto file_size = get_size(handle.get());
			if (file_size < size || offset > size) return false;

			if (!size) {
				size = file_size;
			}

			static constexpr const auto read = [](const handle::native_t handle, const offset_t offset, const lsize_t size, byte_p buffer) noexcept {
				auto status = status_t();
				return NtReadFile(handle, nullptr, nullptr, nullptr, &status, buffer, size, PLARGE_INTEGER(&offset), nullptr);
			};

			if (size > __fileReadWriteLimit) {
				auto parts = aligned<size_t>(size, __fileReadWriteLimit);
				for (size_t i = 0; i < parts.count; i++) {
					const auto current_offset = parts.divided * i;
					read(handle.get(), offset + current_offset, parts.divided, byte_p(_data) + current_offset);
				}

				if (parts.low) {
					const auto current_offset = parts.divided * parts.count;
					read(handle.get(), offset + current_offset, parts.divided, byte_p(_data) + current_offset);
				}
			}
			else {
				read(handle.get(), offset, size, byte_p(_data));
			}

			return true;
		}

		template<typename _t> auto read_memory(offset_t offset = null) const noexcept {
			auto result = _t();
			read_memory(offset, sizeof(_t), &result);
			return result;
		}

		__forceinline auto read() const noexcept {
			auto result = std::vector<byte_t>();

			auto handle = temp_handle(_path, handle_t(), FILE_READ_DATA | SYNCHRONIZE, create_disposion::open_existing);
			if (!handle) _Exit: return result;

			auto size = get_size(handle.get());
			if (!size) goto _Exit;

			result.resize(size);
			if (!read_memory(null, size, result.data())) {
				result.clear();
			}

			goto _Exit;
		}

		__forceinline auto read(byte_p* _data) const noexcept {
			auto size = size_t();
			if (!_data) _Exit: return size;

			auto handle = temp_handle(_path, handle_t(), FILE_READ_DATA | SYNCHRONIZE, create_disposion::open_existing);
			if (!handle) goto _Exit;

			if (!(size = get_size(handle.get()))) goto _Exit;

			auto useless = byte_p(nullptr);
			auto buffer = byte_p(malloc(size));

			if (read_memory(null, size, buffer)) {
				useless = *_data;
				*_data = buffer;
			}
			else {
				useless = buffer;
			}

			if (useless) {
				free(useless);
			}

			goto _Exit;
		}
	};

	static __forceinline auto file_exists(const std::string& path) noexcept {
		return file(path).exists();
	}

	static __forceinline auto get_file_size(const std::string& path) noexcept {
		return file(path).get_size();
	}

	static __forceinline auto erase_file(const std::string& path) noexcept {
		return file(path).erase();
	}

	static __forceinline auto read_file(const std::string& path) noexcept {
		return file(path).read();
	}

	static __forceinline auto read_file(const std::string& path, byte_p* _result) noexcept {
		return file(path).read(_result);
	}

	static __forceinline auto write_file(const std::string& path, const void* data, const size_t size) noexcept {
		return file(path).write(data, size);
	}

	static __forceinline auto write_file(const std::string& path, const std::vector<byte_t>& data) noexcept {
		return file(path).write(data);
	}

	static __forceinline auto get_module_directory(address_t module) noexcept {
		char file_path[__filePathLimit * 2 + 1]{ 0 };
		char file_drive[__filePathLimit + 1]{ 0 };
		char file_folder[__filePathLimit + 1]{ 0 };

		GetModuleFileNameA(HMODULE(module), file_path, sizeof(file_path));
		_splitpath(file_path, file_drive, file_folder, null, null);
		return std::string(file_drive) + std::string(file_folder);
	}

	static __forceinline auto current_directory() noexcept {
		static auto path = get_module_directory(null);
		return path;
	}

	static __forceinline std::string system_directory() noexcept {
		static char path[__filePathLimit + 1]{ 0 };
		if (!*path) {
			GetSystemDirectoryA(path, __filePathLimit);

			strcat(path, "\\");
		}
		return path;
	}

	static __forceinline std::string desktop_directory() noexcept {
		static char path[__filePathLimit + 1]{ 0 };
		if (!*path) {
			SHGetSpecialFolderPathA(HWND_DESKTOP, path, CSIDL_DESKTOP, false);

			strcat(path, "\\");
		}
		return path;
	}
}
