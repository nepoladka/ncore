#pragma once
#include "defines.hpp"
#include "handle.hpp"
#include <windows.h>

#define NCORE_WINREG_MAX_LENGTH_KEY_NAME 0xff
#define NCORE_WINREG_MAX_LENGTH_VALUE_NAME 0x3fff

#define NCORE_WINREG_IMP_TYPE_CHECK(NAME) __forceinline constexpr const auto is_##NAME() const noexcept { return value == NAME; }

namespace ncore::winreg {
	using basekey_t = HKEY;

    static constexpr const auto const __defaultEntryOpenAccess = KEY_ALL_ACCESS;
	static const auto const __entryHandleCloser = handle::handle_t<basekey_t>::closer_t(RegCloseKey);

    struct key_t {
		basekey_t handle;
        std::string name; 
		
		__forceinline auto temp_handle(unsigned open_access = __defaultEntryOpenAccess) const noexcept {
			auto result = handle::handle_t<basekey_t>();

			auto handle = basekey_t(null);
			if (RegOpenKeyExA(this->handle, name.empty() ? nullptr : name.c_str(), null, open_access, &handle) == ERROR_SUCCESS) {
				(result = handle::handle_t<basekey_t>(handle, __entryHandleCloser, false)).close_on_destroy(true);
			}

			return result;
		}
    };

	template<typename data_t = byte_t> struct value_t {
	private:
		lsize_t _length;
		data_t* _data;

		bool _allocated;

	public:
		__forceinline value_t(lsize_t length = null, data_t* data = nullptr, bool allocated = false) noexcept {
			_length = length;
			_data = data;
			_allocated = allocated;
		}

		__forceinline void release() noexcept {
			if (!_allocated) return;
			_allocated = false;

			auto buffer = _data;

			_length = null;
			_data = nullptr;

			if (buffer) return free(buffer);
		}

		__forceinline constexpr auto length() const noexcept {
			return _length;
		}

		__forceinline constexpr auto size() const noexcept {
			return _length;
		}

		__forceinline constexpr auto data() const noexcept {
			return _data;
		}

		__forceinline constexpr auto allocated() const noexcept {
			return _allocated;
		}

		__forceinline constexpr auto empty() const noexcept {
			return !(_data && _length);
		}

		__forceinline constexpr auto valid() const noexcept {
			return _allocated && _data && _length;
		}
	};

	class regentry;
	class regvalue;

	class regvalue {
	public:
		struct type_t {
			struct info_t {
				lsize_t length;
				const char* name;
			};

			enum : ui32_t {
				none,
				string,
				string_expand,
				binary,
				dword,
				dword_le,
				dword_be,
				link,
				string_multi,
				resources,
				resource_descriptor,
				resource_requirements,
				qword,
				qword_le
			}value;

			NCORE_WINREG_IMP_TYPE_CHECK(string);
			NCORE_WINREG_IMP_TYPE_CHECK(string_expand);
			NCORE_WINREG_IMP_TYPE_CHECK(binary);
			NCORE_WINREG_IMP_TYPE_CHECK(dword);
			NCORE_WINREG_IMP_TYPE_CHECK(dword_le);
			NCORE_WINREG_IMP_TYPE_CHECK(dword_be);
			NCORE_WINREG_IMP_TYPE_CHECK(link);
			NCORE_WINREG_IMP_TYPE_CHECK(string_multi);
			NCORE_WINREG_IMP_TYPE_CHECK(resources);
			NCORE_WINREG_IMP_TYPE_CHECK(resource_descriptor);
			NCORE_WINREG_IMP_TYPE_CHECK(resource_requirements);
			NCORE_WINREG_IMP_TYPE_CHECK(qword);
			NCORE_WINREG_IMP_TYPE_CHECK(qword_le);

			__forceinline constexpr type_t(ui32_t type = null) noexcept {
				*ui32_p(this) = type;
			}

			//todo: move names and sizes to defines to allow customization
			__forceinline auto info() const noexcept {
				static constexpr const info_t __infos[] = {
					{0, "none"},
					{0, "string"},
					{0, "string_expand"},
					{0, "binary"},

					{32, "dword"},
					{32, "dword_le"},
					{32, "dword_be"},
					{0, "link"},
					{0, "string_multi"},
					{0, "resources"},
					{0, "resource_descriptor"},
					{0, "resource_requirements"},
					{64, "qword"},
					{64, "qword_le"}
				};

				return __infos[value];
			}
		};

	private:
		const regentry* _entry;

		type_t _type;
		std::string _name;
		lsize_t _length;
		value_t<byte_t> _readed;

	public:
		__forceinline regvalue(const regentry* entry = nullptr, const type_t& type = null, const std::string& name = std::string(), lsize_t length = null) noexcept {
			_entry = entry;
			_type = type;
			_name = name;
			_length = length;
		}

		__forceinline constexpr auto entry() const noexcept {
			return _entry;
		}

		__forceinline constexpr auto name() const noexcept {
			return _name;
		}

		__forceinline constexpr auto size() const noexcept {
			return _length;
		}

		__forceinline constexpr auto length() const noexcept {
			return _length;
		}

		__forceinline auto data(bool force_read = false) const noexcept {
			return read(force_read);
		}

		template<typename _t = byte_t> __forceinline const value_t<_t>& const read(bool force = false) const noexcept;

		//todo
		template<typename _t = byte_t> __forceinline void write(const value_t<_t>& value, bool free_after_use = false) noexcept;

		__forceinline void release() noexcept {
			return _readed.release();
		}
	};

	class regentry {
    public:
        struct info_t {
			struct entry {
				std::string name;
			};

			struct value {
				ui32_t type;
				lsize_t length;
				std::string name;
			};

			std::vector<std::string> entries;
			std::vector<value> values;
        };

	private:
		static auto get_info(const basekey_t& handle) noexcept {
			auto result = info_t();

			TCHAR    achKey[NCORE_WINREG_MAX_LENGTH_KEY_NAME];   // buffer for subkey name
			DWORD    cbName;                   // size of name string 
			TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name 
			DWORD    cchClassName = MAX_PATH;  // size of class string 
			DWORD    cSubKeys = 0;               // number of subkeys 
			DWORD    cbMaxSubKey;              // longest subkey size 
			DWORD    cchMaxClass;              // longest class string 
			DWORD    cValues;              // number of values for key 
			DWORD    cchMaxValue;          // longest value name 
			DWORD    cbMaxValueData;       // longest value data 
			DWORD    cbSecurityDescriptor; // size of security descriptor 
			FILETIME ftLastWriteTime;      // last write time 

			DWORD i, retCode;

			TCHAR  achValue[NCORE_WINREG_MAX_LENGTH_VALUE_NAME];
			DWORD cchValue = NCORE_WINREG_MAX_LENGTH_VALUE_NAME;

			// Get the class name and the value count. 
			retCode = RegQueryInfoKeyA(
				handle,                    // key handle 
				achClass,                // buffer for class name 
				&cchClassName,           // size of class string 
				NULL,                    // reserved 
				&cSubKeys,               // number of subkeys 
				&cbMaxSubKey,            // longest subkey size 
				&cchMaxClass,            // longest class string 
				&cValues,                // number of values for this key 
				&cchMaxValue,            // longest value name 
				&cbMaxValueData,         // longest value data 
				&cbSecurityDescriptor,   // security descriptor 
				&ftLastWriteTime);       // last write time 

			// Enumerate the subkeys, until RegEnumKeyEx fails.

			if (cSubKeys)
			{
				//printf("\nNumber of subkeys: %d\n", cSubKeys);

				for (i = 0; i < cSubKeys; i++)
				{
					cbName = NCORE_WINREG_MAX_LENGTH_KEY_NAME;
					retCode = RegEnumKeyExA(handle, i,
						achKey,
						&cbName,
						NULL,
						NULL,
						NULL,
						&ftLastWriteTime);
					if (retCode == ERROR_SUCCESS)
					{
						result.entries.push_back(achKey);
						//printf(TEXT("(%d) %s\n"), i + 1, achKey);
					}
				}
			}

			// Enumerate the key values. 

			if (cValues)
			{
				//printf("\nNumber of values: %d\n", cValues);

				for (i = 0, retCode = ERROR_SUCCESS; i < cValues; i++)
				{
					cchValue = NCORE_WINREG_MAX_LENGTH_VALUE_NAME;
					achValue[0] = '\0';
					DWORD type = null;
					DWORD len = 0;
					retCode = RegEnumValueA(handle, i, achValue, &cchValue, nullptr, &type, nullptr, &len);

					if (retCode == ERROR_SUCCESS)
					{
						//printf(TEXT("(%d) %s\n"), i + 1, achValue);
						result.values.push_back({ type, len, achValue });
					}
				}
			}

			return result;
		}

        key_t _key;

    public:
        __forceinline regentry() = default;

        __forceinline regentry(const key_t& key) noexcept : _key(key) { return; }

        __forceinline regentry(const basekey_t& handle, const std::string& name = std::string()) noexcept : _key({ handle, name }) { return; }

		__forceinline constexpr auto valid() const noexcept {
			return _key.temp_handle().get();
		}

        template<typename _t> __forceinline auto read_value(const std::string& name, lsize_t length = null) const noexcept {
			auto result = value_t<_t>();

			auto handle = _key.temp_handle();
			if (!handle) _Exit: return result;

			if (!length) {
				RegQueryValueExA(handle.get(), name.c_str(), nullptr, nullptr, nullptr, LPDWORD(&length));
				if (!length) goto _Exit;
			}

			auto buffer = byte_p(malloc(length));
			if (RegQueryValueExA(handle.get(), name.c_str(), nullptr, nullptr,  buffer, LPDWORD(&length)) == ERROR_SUCCESS) {
				result = value_t<_t>(length, (_t*)buffer, true);
			}
			else {
				free(buffer);
			}

            goto _Exit;
        }

		template<typename _t> __forceinline auto write_value(const std::string& name, const value_t<_t>& value, bool free_after_use = false) noexcept {
			//todo
		}

		__forceinline auto name() const noexcept {
			/*static constexpr const char* const __names[] = {
				"HKEY_CLASSES_ROOT",
				"HKEY_CURRENT_USER",
				"HKEY_LOCAL_MACHINE",
				"HKEY_USERS",
				"HKEY_PERFORMANCE_DATA",
				"HKEY_PERFORMANCE_TEXT",
				"HKEY_PERFORMANCE_NLSTEXT",
				"HKEY_CURRENT_CONFIG",
				"HKEY_DYN_DATA",
				"HKEY_CURRENT_USER_LOCAL_SETTINGS"
			};

			static auto get_base_name = [](basekey_t handle) {
				switch (handle) {
				case HKEY_CLASSES_ROOT: return 0;
				};
			};*/

			return _key.name;
		}

		__forceinline auto handle() const noexcept {
			return _key.handle;
		}

        __forceinline auto get_entries() const noexcept {
            auto result = std::vector<regentry>();

			auto handle = _key.temp_handle();
			if (!handle) _Exit: return result;

			auto entries = get_info(handle.get()).entries;
			for (auto& entry : entries) {
				result.push_back(regentry(_key.handle, entry));
			}

            goto _Exit;
        }

        __forceinline auto get_values() const noexcept {
			auto result = std::vector<regvalue>();

			auto handle = _key.temp_handle();
			if (!handle) _Exit: return result;

			auto values = get_info(handle.get()).values;
			for (auto& value : values) {
				result.push_back(regvalue(this, value.type, value.name, value.length));
			}

			goto _Exit;
        }

		__forceinline auto search_entry(const std::string& name) const noexcept {
			auto entries = get_entries();
			for (auto& entry : entries) {
				if (entry._key.name == name) return entry;
			}

			return regentry();
		}

		__forceinline auto search_value(const std::string& name) const noexcept {
			auto values = get_values();
			for (auto& value : values) {
				if (value.name() == name) return value;
			}

			return regvalue();
		}
	};

	template<typename _t> const value_t<_t>& const regvalue::read(bool force) const noexcept {
		if (_readed.valid() && !force) _Exit: return *((value_t<_t>*) & _readed);

		auto previous = _readed;
		*((value_t<_t>*) & _readed) = _entry->read_value<_t>(_name, _length);
		previous.release();

		goto _Exit;
	}

	//auto readed = ncore::winreg::read<char>(HKEY_CURRENT_USER, "SOFTWARE\\JavaSoft\\DeploymentProperties", "deployment.browser.path");
	template<typename _t> static __forceinline auto read(basekey_t base, const std::string& key, const std::string& value) noexcept {
		return regentry(base, key).read_value<_t>(value);
	}

	//ncore::winreg::write<char>(HKEY_CURRENT_USER, "SOFTWARE\\JavaSoft\\DeploymentProperties", "deployment.browser.path", { 8, "1234567" }, false);
	template<typename _t> static __forceinline auto write(basekey_t base, const std::string& key, const std::string& value, const value_t<_t>& data, bool free_after_use = false) noexcept {
		return regentry(base, key).write_value<_t>(value, data, free_after_use);
	}
}

#ifdef NCORE_WINREG_IMP_TYPE_CHECK
#undef NCORE_WINREG_IMP_TYPE_CHECK
#endif
