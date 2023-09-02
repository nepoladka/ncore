#pragma once
#include "thread.hpp"

#define is_lowercase_input(VK_SHIFT_PRESSED) ((bool)(!((((is_key_pressed(VK_CAPITAL) & 0x0001) != 0) && !((bool)(VK_SHIFT_PRESSED))) ? (true) : ((!((is_key_pressed(VK_CAPITAL) & 0x0001) != 0) && ((bool)(VK_SHIFT_PRESSED))) ? true : false))))

namespace ncore {
	class input {
	private:
		using get_key_state_t = get_procedure_t(short, , int);
		get_key_state_t _get_key_state = nullptr;

		static constexpr const char* const __keyNames[] = {
            "UNKNOWN", "VK_LBUTTON", "VK_RBUTTON", "VK_CANCEL", "VK_MBUTTON", "VK_XBUTTON1", "VK_XBUTTON2", "RESERVED",
            "VK_BACK", "VK_TAB", "RESERVED", "RESERVED", "VK_CLEAR", "VK_RETURN", "UNASSIGNED", "UNASSIGNED",
			"VK_SHIFT", "VK_CONTROL", "VK_MENU", "VK_PAUSE", "VK_CAPITAL", "VK_KANA", "VK_IME_ON", "VK_JUNJA", "VK_FINAL", "VK_KANJI", "VK_IME_OFF",
			"VK_ESCAPE", "VK_CONVERT", "VK_NONCONVERT", "VK_ACCEPT", "VK_MODECHANGE", "VK_SPACE", "VK_PRIOR", "VK_NEXT", "VK_END", "VK_HOME", "VK_LEFT", "VK_UP",
			"VK_RIGHT", "VK_DOWN", "VK_SELECT", "VK_PRINT", "VK_EXECUTE", "VK_SNAPSHOT", "VK_INSERT", "VK_DELETE", "VK_HELP",
			"VK_0", "VK_1", "VK_2", "VK_3", "VK_4", "VK_5", "VK_6", "VK_7", "VK_8", "VK_9",
			"UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED",
			"VK_A", "VK_B", "VK_C", "VK_D", "VK_E", "VK_F", "VK_G", "VK_H", "VK_I", "VK_J", "VK_K", "VK_L", "VK_M", "VK_N",
			"VK_O", "VK_P", "VK_Q", "VK_R", "VK_S", "VK_T", "VK_U", "VK_V", "VK_W", "VK_X", "VK_Y", "VK_Z", "VK_LWIN", "VK_RWIN",
			"VK_APPS", "RESERVED", "VK_SLEEP", "VK_NUMPAD0", "VK_NUMPAD1", "VK_NUMPAD2", "VK_NUMPAD3", "VK_NUMPAD4", "VK_NUMPAD5",
			"VK_NUMPAD6", "VK_NUMPAD7", "VK_NUMPAD8", "VK_NUMPAD9", "VK_MULTIPLY", "VK_ADD",
			"VK_SEPARATOR", "VK_SUBTRACT", "VK_DECIMAL", "VK_DIVIDE", "VK_F1", "VK_F2", "VK_F3", "VK_F4", "VK_F5",
			"VK_F6", "VK_F7", "VK_F8", "VK_F9", "VK_F10", "VK_F11", "VK_F12", "VK_F13", "VK_F14", "VK_F15", "VK_F16",
			"VK_F17", "VK_F18", "VK_F19", "VK_F20", "VK_F21", "VK_F22", "VK_F23", "VK_F24", "VK_NAVIGATION_VIEW",
			"VK_NAVIGATION_MENU", "VK_NAVIGATION_UP", "VK_NAVIGATION_DOWN", "VK_NAVIGATION_LEFT", "VK_NAVIGATION_RIGHT",
			"VK_NAVIGATION_ACCEPT", "VK_NAVIGATION_CANCEL", "VK_NUMLOCK", "VK_SCROLL", "VK_OEM_NEC_EQUAL", "VK_OEM_FJ_MASSHOU",
			"VK_OEM_FJ_TOUROKU", "VK_OEM_FJ_LOYA", "VK_OEM_FJ_ROYA", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED",
			"UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "UNASSIGNED", "VK_LSHIFT", "VK_RSHIFT", "VK_LCONTROL",
			"VK_RCONTROL", "VK_LMENU", "VK_RMENU", "VK_BROWSER_BACK", "VK_BROWSER_FORWARD", "VK_BROWSER_REFRESH",
			"VK_BROWSER_STOP", "VK_BROWSER_SEARCH", "VK_BROWSER_FAVORITES", "VK_BROWSER_HOME", "VK_VOLUME_MUTE",
			"VK_VOLUME_DOWN", "VK_VOLUME_UP", "VK_MEDIA_NEXT_TRACK", "VK_MEDIA_PREV_TRACK", "VK_MEDIA_STOP",
			"VK_MEDIA_PLAY_PAUSE", "VK_LAUNCH_MAIL", "VK_LAUNCH_MEDIA_SELECT", "VK_LAUNCH_APP1", "VK_LAUNCH_APP2",
			"RESERVED", "RESERVED", "VK_OEM_1", "VK_OEM_PLUS", "VK_OEM_COMMA", "VK_OEM_MINUS", "VK_OEM_PERIOD", "VK_OEM_2", "VK_OEM_3",
			"RESERVED", "RESERVED", "VK_GAMEPAD_A", "VK_GAMEPAD_B", "VK_GAMEPAD_X", "VK_GAMEPAD_Y", "VK_GAMEPAD_RIGHT_SHOULDER",
			"VK_GAMEPAD_LEFT_SHOULDER", "VK_GAMEPAD_LEFT_TRIGGER", "VK_GAMEPAD_RIGHT_TRIGGER", "VK_GAMEPAD_DPAD_UP", "VK_GAMEPAD_DPAD_DOWN",
			"VK_GAMEPAD_DPAD_LEFT", "VK_GAMEPAD_DPAD_RIGHT", "VK_GAMEPAD_MENU", "VK_GAMEPAD_VIEW", "VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON",
			"VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON", "VK_GAMEPAD_LEFT_THUMBSTICK_UP", "VK_GAMEPAD_LEFT_THUMBSTICK_DOWN", "VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT",
			"VK_GAMEPAD_LEFT_THUMBSTICK_LEFT", "VK_GAMEPAD_RIGHT_THUMBSTICK_UP", "VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN", "VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT",
			"VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT", "VK_OEM_4", "VK_OEM_5", "VK_OEM_6", "VK_OEM_7", "VK_OEM_8", "RESERVED", "VK_OEM_AX", "VK_OEM_102",
			"VK_ICO_HELP", "VK_ICO_00", "VK_PROCESSKEY", "VK_ICO_CLEAR", "VK_PACKET", "UNASSIGNED", "VK_OEM_RESET", "VK_OEM_JUMP", "VK_OEM_PA1",
			"VK_OEM_PA2", "VK_OEM_PA3", "VK_OEM_WSCTRL", "VK_OEM_CUSEL", "VK_OEM_ATTN", "VK_OEM_FINISH", "VK_OEM_COPY", "VK_OEM_AUTO",
			"VK_OEM_ENLW", "VK_OEM_BACKTAB", "VK_ATTN", "VK_CRSEL", "VK_EXSEL", "VK_EREOF", "VK_PLAY", "VK_ZOOM", "VK_NONAME",
			"VK_PA1", "VK_OEM_CLEAR", "RESERVED" };

	public:
		union key_info {
			int index;

			__forceinline constexpr key_info(int index = null) {
				this->index = index;
			}

			__forceinline constexpr std::string name() const noexcept {
				return get_key_name(index);
			}
		};

		__declspec(align(4)) struct mouse_position {
			i32_t x = 0, y = 0;

			__forceinline ui64_t summary() const noexcept { return *(ui64_t*)this; }
		};
		
		using key_callback_t = get_procedure_t(void, , int i, bool s);
		using move_callback_t = get_procedure_t(void, , int x, int y);

		static __forceinline bool is_key_pressed(int index) noexcept {
			return (GetKeyState(index) & 0x8000) != 0;
		}

		__forceinline bool is_key_pressed_ex(int index) const noexcept {
			return (_get_key_state(index) & 0x8000) != 0;
		}

		static __forceinline mouse_position get_mouse_position() noexcept {
			auto result = mouse_position();
			GetCursorPos(LPPOINT(&result));
			return result;
		}

		static __forceinline bool wait_for_key(int index, int delay = 50, ui32_t timeout = -1) noexcept {
			struct local {
				int key;
				int delay;

				static __declspec(noinline) void waiting_thread(local* data) {
					while (!is_key_pressed(data->key)) {
						ncore::thread::sleep(data->delay);
					}
				}
			};

			auto thread_info = local{ index, delay };
			auto waiting_thread = ncore::thread::create(local::waiting_thread, &thread_info);
			if (waiting_thread.wait(timeout) != WAIT_OBJECT_0) {
				waiting_thread.terminate();
				return false;
			}

			return true;
		}

		static __forceinline constexpr const char* get_key_name(int index) noexcept {
			return __keyNames[index];
		}

		static __forceinline HKL get_current_keyboard_layout() noexcept {
			struct local {
				static __declspec(noinline) void get_layout(HKL* _result) noexcept {
					if (_result) {
						*_result = GetKeyboardLayout(null);
					}
				}
			};

			auto result = HKL(null);
			thread::create(local::get_layout, &result).wait();
			return result;
		}

		template<typename char_t = char> static __forceinline char_t get_key_char(int index) noexcept {
			byte_t states[0xff]{ 0 };
			states[VK_CAPITAL] = is_key_pressed(VK_CAPITAL);
			states[VK_SHIFT] = is_key_pressed(VK_SHIFT);

			auto layout = get_current_keyboard_layout();
			auto scan = MapVirtualKeyExW(index, MAPVK_VK_TO_VSC, layout);

			auto result = ui16_t(null);
			auto status = sizeof(char_t) == 1 ?
				ToAsciiEx(index, scan, states, &result, NULL, layout) :
				ToUnicodeEx(index, scan, states, (wchar_t*)&result, 1, NULL, layout);

			return *(char_t*)&result;
		}

		static __forceinline char vk_to_ascii(int index) noexcept {
			return get_key_char<char>(index);
		}

		static __forceinline wchar_t vk_to_unicode(int index) noexcept {
			return get_key_char<wchar_t>(index);
		}

	private:
		__declspec(align(2)) struct state {
			bool changed, pressed;
		}_processed_states[0xff];

		bool _previous_states[0xff];
		bool _current_states[0xff];

		key_callback_t _key_callback = nullptr;
		move_callback_t _move_callback = nullptr;

		unsigned _thread_delay;
		thread _thread;

		__forceinline state get_state(int index) noexcept {
			auto changed = _previous_states[index] != (_current_states[index] = is_key_pressed_ex(index));
			if (changed) {
				_previous_states[index] = _current_states[index];
			}

			return { changed, _current_states[index] };
		}

		static __declspec(noinline) void checking_thread(input* instance) noexcept {
			mouse_position previous_position, current_position;

		_Begin: 
			if(instance->_key_callback) for (short i = 0; i < size_of_array(_processed_states); i++) {
				auto state = instance->_processed_states[i] = instance->get_state(i);
				if (state.changed) {
					instance->_key_callback(i, state.pressed);
				}
			}

			if(instance->_move_callback) if ((current_position = instance->get_mouse_position()).summary() != previous_position.summary()) {
				previous_position = current_position;

				instance->_move_callback(current_position.x, current_position.y);
			}

			ncore::thread::sleep(instance->_thread_delay);

			goto _Begin;
		}

	public:
		__forceinline input(key_callback_t key_callback, move_callback_t move_callback, unsigned thread_delay = 50, bool use_native = true) noexcept {
			set_key_callback(key_callback);
			set_move_callback(move_callback);

			delay(thread_delay, true);

			_get_key_state = use_native ?
				get_key_state_t(GetProcAddress(LoadLibraryA("win32u.dll"), "NtUserGetKeyState")) :
				GetKeyState;

			_thread = ncore::thread::create(checking_thread, this, nullptr, false, THREAD_PRIORITY_IDLE);
		}

		__forceinline ~input() noexcept {
			if (!_thread.alive()) return;

			_thread.terminate();
			_thread = thread();
		}

		__forceinline constexpr key_callback_t set_key_callback(key_callback_t callback) noexcept {
			auto previous = _key_callback;
			_key_callback = callback;
			return previous;
		}

		__forceinline constexpr move_callback_t set_move_callback(move_callback_t callback) noexcept {
			auto previous = _move_callback;
			_move_callback = callback;
			return previous;
		}

		__forceinline constexpr unsigned delay(unsigned delay, bool set = false) noexcept {
			if (set) {
				_thread_delay = delay;
			}

			return _thread_delay;
		}

		__forceinline bool suspend() noexcept {
			return _thread.suspend();
		}

		__forceinline bool resume() noexcept {
			return _thread.resume();
		}
	};
}
