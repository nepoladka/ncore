#pragma once
#include "file.hpp"
#include "silent_library.hpp"
#include "includes/kdu/kdu.dll.h"
#include "includes/kdu/drv64.dll.h"
#include <ntstatus.h>

namespace ncore {
	using nt_status_t = NTSTATUS;

	static constexpr const char const __drv64dllName[] = { "drv64.dll" };

	class kernel_map {
	private:
		using map_driver_t = nt_status_t(*)(const address_t, id_t);
		using map_file_image_t = const address_t(*)(const address_t);
		using unmap_file_image_t = void (*)(const address_t);

		silent_library* _library = nullptr;
		bool _drv64_created = false;

		__forceinline kernel_map() noexcept {
			if (!(_drv64_created = create_drv64_dll())) return;

			_library = silent_library::load((byte_t*)kdu::files::kdu_dll, sizeof(kdu::files::kdu_dll));
		}

		__forceinline ~kernel_map() noexcept {
			if (initialized()) {
				auto library = _library;
				_library = nullptr;
				library->unload();
			}

			if (_drv64_created) {
				_drv64_created = !remove_drv64_dll();
			}
		}

		__forceinline constexpr bool initialized() const noexcept {
			return _library;
		}

		template<typename _t> __forceinline constexpr _t get_export(const char* name) const noexcept {
			if (!initialized()) return _t();

			return _t(_library->search_export(name));
		}

		__forceinline bool create_drv64_dll() const noexcept {
			return ncore::write_file(__drv64dllName, kdu::files::drv64_dll, sizeof(kdu::files::drv64_dll));
		}

		__forceinline bool remove_drv64_dll() const noexcept {
			return ncore::erase_file(__drv64dllName);
		}

		__forceinline nt_status_t map_driver(const address_t image, id_t provider_id) const noexcept {
			auto procedure = get_export<map_driver_t>("MapDriver");
			if (!procedure) return STATUS_PROCEDURE_NOT_FOUND;

			return procedure(image, provider_id);
		}

		__forceinline const address_t map_file_image(const address_t file_data) const noexcept {
			auto procedure = get_export<map_file_image_t>("MapFileImage");
			if (!procedure) return NULL;

			return procedure(file_data);
		}

		__forceinline void unmap_file_image(const address_t image) const noexcept {
			auto procedure = get_export<unmap_file_image_t>("UnapFileImage");
			if (!procedure) return;

			return procedure(image);
		}

	public:
		static __forceinline nt_status_t map_from_memory(const address_t file, id_t vulnerability_provider = 0) {
			const auto internal = kernel_map();
			if (!internal.initialized()) return STATUS_DLL_INIT_FAILED;

			auto image = internal.map_file_image(file);
			if (!image) return STATUS_ACCESS_DENIED;

			auto status = internal.map_driver(image, vulnerability_provider);

			internal.unmap_file_image(image);

			return status;
		}

		static __forceinline nt_status_t map_from_file(const std::string& path, id_t vulnerability_provider = 0) {
			auto data = byte_p(nullptr);
			if (!read_file(path, &data)) return STATUS_FILE_NOT_AVAILABLE;

			return map_from_memory(data, vulnerability_provider);
		}
	};
}
