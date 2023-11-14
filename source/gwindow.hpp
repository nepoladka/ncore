#pragma once

#ifdef NCORE_GWINDOW_GL_PATH
#ifndef NCORE_GWINDOW_NO_GL_SUBINIT
#ifndef NCORE_GWINDOW_GL_SUBINIT
#error [when you're using custom gl path (for example - glad), you must define NCORE_GWINDOW_GL_SUBINIT and implement the gl initialization procedure, but if your gl haven't this one, you can define NCORE_GWINDOW_NO_GL_SUBINIT and it's error is gone]
#endif
#endif
#else
#define NCORE_GWINDOW_GL_PATH <gl/gl.h>
#endif

#include NCORE_GWINDOW_GL_PATH

#ifdef NCORE_GWINDOW_INCLUDE_OGL_LIBRARY
#pragma comment(lib, "opengl32.lib")
#endif

#include "thread.hpp"
#include "utils.hpp"
#include "dimension_vector.hpp"

//to work with multiply threads, you need this in imconfig.h:
//  struct ImGuiContext;
//  extern thread_local ImGuiContext* ImGuiTLContext;
//  #define GImGui ImGuiTLContext
//and this in one of your .cpp files:
//  thread_local ImGuiContext* ImGuiTLContext;

#define IMGUI_DEFINE_MATH_OPERATORS
#include "includes/gwindow/imgui/imgui.h"
#include "includes/gwindow/imgui/imgui_internal.h"
#include "includes/gwindow/imgui/backends/imgui_impl_win32.h"
#include "includes/gwindow/imgui/backends/imgui_impl_opengl3.h"

#ifndef NCORE_GWINDOW_DEFAULT_POSITION
#define NCORE_GWINDOW_DEFAULT_POSITION CW_USEDEFAULT, CW_USEDEFAULT
#endif NCORE_GWINDOW_DEFAULT_POSITION

#ifndef NCORE_GWINDOW_DEFAULT_ICON_NAME
#define NCORE_GWINDOW_DEFAULT_ICON_NAME "NGLW_ICON"
#endif

#ifdef NCORE_GWINDOW_UTILS
#define IMGUIFILEDIALOG_NO_EXPORT
#include "includes/gwindow/imgui/file_dialog/imgui_file_dialog.h"

#ifndef NCORE_GWINDOW_DIALOGS_DELAY
#define NCORE_GWINDOW_DIALOGS_DELAY 25 //ms
#endif

#ifndef NCORE_GWINDOW_RESET_KEY
#define NCORE_GWINDOW_RESET_KEY VK_ESCAPE
#endif

#ifndef NCORE_GWINDOW_PBAR_ANIMATION_DELAY
#define NCORE_GWINDOW_PBAR_ANIMATION_DELAY 800 //ms
#endif

#ifndef NCORE_GWINDOW_PBAR_ANIMATION_CHAR
#define NCORE_GWINDOW_PBAR_ANIMATION_CHAR "*"
#endif

#ifndef NCORE_GWINDOW_PBAR_ANIMATION_CHARS_COUNT
#define NCORE_GWINDOW_PBAR_ANIMATION_CHARS_COUNT char(3)
#endif

#define qdialog_frame_parameters    ncore::gwindow& window, ncore::gwindow::gui::window_t& gui, const ncore::qdialog& dialog, const ncore::qdialog::window_inner_info& inner
#define qdialog_window_parameters   ncore::gwindow& window, const ncore::qdialog& dialog
#define qdialog_render_procedure()  static void render(qdialog_frame_parameters) noexcept
#define qdialog_open_procedure()    static void open(qdialog_window_parameters) noexcept 
#define qdialog_close_procedure()   static void close(qdialog_window_parameters) noexcept 
#define qdialog_data(TYPE)          dialog.callback_data<TYPE>()

#define qda_none            { }
#define qda_ok              { "Ok" }
#define qda_cancel          { "Cancel" }
#define qda_ok_cancel       { "Ok", "Cancel" }
#define qda_yes_no          { "Yes", "No" }
#define qda_yes_no_cancel   { "Yes", "No", "Cancel" }
#endif

#include <mutex>

namespace ImGui {
    namespace Utils {
        static __forceinline void CenteredText(const char* fmt, ...) noexcept {
            va_list list;
            va_start(list, fmt);

            char buffer[0x400]{ 0 };
            vsnprintf(buffer, sizeof(buffer), fmt, list);

            va_end(list);

            SetCursorPosX(GetCurrentWindow()->Size.x * .5f - CalcTextSize(buffer).x * .5f);
            return TextEx(buffer);
        }

        static __forceinline bool Tooltip(const char* fmt, ...) noexcept {
            if (!ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) return false;

            va_list list;
            va_start(list, fmt);

            char buffer[0x400]{ 0 };
            vsnprintf(buffer, sizeof(buffer), fmt, list);

            va_end(list);

            ImGui::SetTooltip(buffer);

            return true;
        }

        static __forceinline bool IsNextItemVisible(ImGuiWindow* window = nullptr) noexcept {
            if (!window) {
                window = ImGui::GetCurrentWindow();
            }

            const auto scroll_pos = window->Scroll.y;
            const auto next_item_pos = (window->DC.CursorPos - window->Pos + window->Scroll).y;
            const auto window_height = window->Size.y;

            return (next_item_pos - scroll_pos) < (window_height * 1.25f) && next_item_pos > (scroll_pos - window_height);
        }
    }

    using namespace Utils;
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace ncore {
    static constexpr const auto const __defaultGuiFrameFlags = ImGuiWindowFlags(ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);
    static constexpr const auto const __defaultIconName = NCORE_GWINDOW_DEFAULT_ICON_NAME;

#ifdef NCORE_GWINDOW_UTILS
    static constexpr const auto const __dialogWindowDelay = NCORE_GWINDOW_DIALOGS_DELAY;
    static constexpr const auto const __pbarAnimationDelay = NCORE_GWINDOW_PBAR_ANIMATION_DELAY;
    static constexpr const auto const __pbarAnimationChar = NCORE_GWINDOW_PBAR_ANIMATION_CHAR;
    static constexpr const auto const __pbarAnimationCharsCount = char(NCORE_GWINDOW_PBAR_ANIMATION_CHARS_COUNT - 1);

    extern ui32_t* dialogs_count; //pointer to your dialogs count value
#endif

    class glwindow {
    public:
        using rect_t = RECT;
        using handle_t = HWND;
        using class_t = WNDCLASSA;
        using handler_t = WNDPROC;

        struct hint_t {
            enum : byte_t {
                resizable, top_most, hidden, transparent
            }name;

            ui64_t value;
        };

        __forceinline glwindow() = default;

    protected:
        std::string _name;
        handle_t _handle;
        class_t _class;
        HDC _drawing_context;
        HGLRC _render_context;

        bool _alive;

        handler_t _event_handler;

        __forceinline __fastcall glwindow(const std::string& name, handle_t handle, class_t win_class, HDC dc, HGLRC rc) noexcept {
            _name = name;
            _handle = handle;
            _class = win_class;
            _drawing_context = dc;
            _render_context = rc;
            _alive = true;
        }

        static __forceinline ui32_t get_style(handle_t handle, bool ex = false) noexcept {
            return GetWindowLongA(handle, ex ? GWL_EXSTYLE : GWL_STYLE);
        }

        static __forceinline ui32_t get_dpi(handle_t handle) noexcept {
            return GetDpiForWindow(handle);
        }

        static __forceinline rect_t rect_for_dpi(ui32_t dpi, ui32_t style, ui32_t ex_style, const rect_t& rect) noexcept {
            auto rectangle = rect;
            AdjustWindowRectExForDpi(&rectangle, style, false, ex_style, dpi);
            return rectangle;
        }

        static __forceinline rect_t rect_for(ui32_t style, ui32_t ex_style, const rect_t& rect) noexcept {
            auto rectangle = rect;
            AdjustWindowRectEx(&rectangle, style, false, ex_style);
            return rectangle;
        }

    public:
        __forceinline auto get_dpi() const noexcept {
            return get_dpi(_handle);
        }

        __forceinline auto get_style(bool ex = false) const noexcept {
            return get_style(_handle, ex);
        }

        __forceinline auto get_size() const noexcept {
            struct { vec2i32 _unused, size; } area;
            GetClientRect(_handle, LPRECT(&area));
            return area.size;
        }

        __forceinline auto get_position() const noexcept {
            auto position = vec2i32();
            ClientToScreen(_handle, LPPOINT(&position));
            return position;
        }

    protected:
        __forceinline rect_t rect_for_dpi(const rect_t& rect) const noexcept {
            return rect_for_dpi(get_dpi(), get_style(), get_style(true), rect);
        }

        __forceinline vec2i32 size_for_dpi(const vec2i32& size) const noexcept {
            auto rect = rect_for_dpi({ null, null, size.x, size.y });
            return { rect.right - rect.left, rect.bottom - rect.top };
        }

        __forceinline vec2i32 position_for_dpi(const vec2i32& position) const noexcept {
            auto rect = rect_for_dpi({ position.x, position.y, position.x, position.y });
            return { rect.left, rect.top };
        }

    public:
        static __forceinline bool __fastcall create(glwindow* _window, handler_t event_handler, const std::string& title, const vec2i32& position, const vec2i32& size, handle_t parent = nullptr, const std::vector<hint_t>& hints = std::vector<hint_t>(), const char* icon_name = nullptr) noexcept {
            static auto base = GetModuleHandleA(nullptr);

            if (!_window) _Fail: return false;

            auto window_class = WNDCLASSA{
                CS_OWNDC,
                event_handler,
                null, null,
                base,
                nullptr,
                LoadCursorA(null, IDC_ARROW),
                null, nullptr,
                title.c_str()
            };

            if (!icon_name) {
                icon_name = __defaultIconName;
            }

            if (!(window_class.hIcon = HICON(LoadImageA(base, icon_name, IMAGE_ICON, null, null, LR_DEFAULTSIZE | LR_SHARED)))) {
                window_class.hIcon = HICON(LoadImageA(nullptr, IDI_APPLICATION, IMAGE_ICON, null, null, LR_DEFAULTSIZE | LR_SHARED));
                //window_class.hIcon = LoadIconA(null, IDI_WINLOGO);
            }

            if (!RegisterClassA(&window_class)) goto _Fail;

            auto show = true;

            auto ex_style = ui32_t(null);
            auto style = ui32_t(WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPSIBLINGS | WS_CLIPCHILDREN);
            for (auto& hint : hints) {
                switch (hint.name) {
                case hint_t::resizable:
                    if (hint.value == false) {
                        style &= ~(WS_MAXIMIZEBOX | WS_THICKFRAME);
                    }
                    break;

                case hint_t::top_most:
                    if (hint.value == true) {
                        ex_style = WS_EX_TOPMOST;
                    }
                    break;

                case hint_t::hidden:
                    show = (hint.value == false);
                    break;

                case hint_t::transparent:
                    __debugbreak(); //todo: ...
                    break;

                default: break;
                }
            }

            auto window_handle = CreateWindowExA(ex_style, title.c_str(), title.c_str(),
                style, position.x, position.y, size.x, size.y, parent, nullptr, base, nullptr);

            if (!window_handle) goto _Fail;

            DragAcceptFiles(window_handle, true);

            auto drawing_context = GetDC(window_handle);

            auto pixel_format_descriptor = PIXELFORMATDESCRIPTOR{
                WORD(sizeof(PIXELFORMATDESCRIPTOR)),
                WORD(1),
                DWORD(PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL),
                BYTE(PFD_TYPE_RGBA),
                BYTE(32)
            };

            auto pixel_format = ChoosePixelFormat(drawing_context, &pixel_format_descriptor);
            if (!pixel_format) goto _Fail;

            if (!SetPixelFormat(drawing_context, pixel_format, &pixel_format_descriptor)) goto _Fail;

            DescribePixelFormat(drawing_context, pixel_format, sizeof(PIXELFORMATDESCRIPTOR), &pixel_format_descriptor);

            auto render_context = wglCreateContext(drawing_context);
            wglMakeCurrent(drawing_context, render_context);

            auto& window = *_window = glwindow(title, window_handle, window_class, drawing_context, render_context);

#ifdef NCORE_GWINDOW_GL_SUBINIT
            NCORE_GWINDOW_GL_SUBINIT;
#endif

            ShowWindowAsync(window_handle, SW_SHOW * show); //если вызвать ShowWindow обычную тогда почемуто отсос и сообщения не приходят
            //UpdateWindow(window_handle); //если эту хуйню вызвать то окно нахуй больше никогда само не будет обновляться(

            return true;
        }

        __forceinline void handle_events() noexcept {
            auto message = MSG();
            while (_alive) {
                if (GetMessageA(&message, _handle, null, null) <= null) break;

                TranslateMessage(&message);
                DispatchMessageA(&message);
            }
        }

        __forceinline void terminate() noexcept {
            _alive = false;

            __try {
                wglMakeCurrent(null, null);
                wglDeleteContext(_render_context);

                DestroyWindow(_handle);
                UnregisterClassA(_name.c_str(), _class.hInstance);

                ReleaseDC(_handle, _drawing_context);
            } __endtry;

            _handle = nullptr;
            _drawing_context = nullptr;
            _render_context = nullptr;
        }

        __forceinline void close() noexcept {
            _alive = false;
            SendMessageA(_handle, WM_CLOSE, null, null);
        }

        __forceinline void resize(long width, long height) noexcept {
            static constexpr auto flags = ui32_t(SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOZORDER);
            auto size = size_for_dpi({ width, height });
            SetWindowPos(_handle, HWND_TOP, null, null, size.x, size.y, flags);
        }

        __forceinline void move(long x, long y) noexcept {
            static constexpr auto flags = ui32_t(SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
            auto position = position_for_dpi({ x, y });
            SetWindowPos(_handle, null, position.x, position.y, null, null, flags);
        }

        __forceinline constexpr auto handle() const noexcept {
            return _handle;
        }
    };

    class gwindow : private glwindow {
    public:
        using hint_t = glwindow::hint_t;
        using gctx_t = ImGuiContext*;
        using callback_t = get_procedure_t(void, , gwindow&, void*);

    private:
        enum class windows_list_action_t {
            get, add, remove
        };

        struct creation_data_t {
            gwindow** window;

            std::string title;
            vec2i32 size;
            i32_t delay;
            callback_t render_callback, open_callback, close_callback;
            void* callback_data;
            std::vector<hint_t> hints;

            int result = -1;
        };

        gctx_t _context;

        ncore::vec2i32 _size, _position;
        i32_t _delay;

        callback_t _render_callback;
        callback_t _close_callback;
        void* _callback_data;

    public:
        __forceinline gwindow() = default;

        __forceinline constexpr auto handle() const noexcept {
            return glwindow::handle();
        }

        __forceinline constexpr auto& render_callback() noexcept {
            return _render_callback;
        }

        __forceinline constexpr auto render_callback() const noexcept {
            return _render_callback;
        }

    private:
        __forceinline void render() noexcept {
            __try {
                if (_render_callback) {
                    _render_callback(*this, _callback_data);
                }

                if (_delay) {
                    ncore::thread::sleep(_delay);
                }
            } __endtry;
        }

        static __forceinline auto process_windows_list(void* data, windows_list_action_t action) noexcept {
            static auto list = new std::vector<gwindow*>();
            static auto mutex = new std::mutex();

            void* result = nullptr;
            if (!data) _Exit: return result;

            mutex->lock();

            switch (action) {
            case windows_list_action_t::get:
                for (auto window : *list) {
                    if (window->handle() != data) continue;

                    result = window;
                    break;
                }
                break;

            case windows_list_action_t::add:
                for (auto i = index_t(0), j = count_t(list->size()); i < j; i++) {
                    if (list->at(i)->_handle != ((gwindow*)data)->_handle) continue;

                    result = data;
                    break;
                }

                if (!result) {
                    list->push_back((gwindow*)data);
                }

                break;

            case windows_list_action_t::remove:
                for (auto i = index_t(0), j = count_t(list->size()); i < j; i++) {
                    if (list->at(i)->_handle != data) continue;

                    list->erase(list->begin() + i);
                    break;
                }
                break;

            default: break;
            }

            mutex->unlock();

            goto _Exit;
        }

        static __forceinline auto get_window(glwindow::handle_t handle) noexcept {
            return (gwindow*)process_windows_list(handle, windows_list_action_t::get);
        }

        static __forceinline void register_window(gwindow* window) noexcept {
            process_windows_list(window, windows_list_action_t::add);
        }

        static __forceinline void unregister_window(handle_t handle) noexcept {
            process_windows_list(handle, windows_list_action_t::remove);
        }

        static __declspec(noinline) LRESULT __stdcall event_handler(HWND handle, UINT message, WPARAM w, LPARAM l) noexcept {
            if (ImGui_ImplWin32_WndProcHandler(handle, message, w, l)) return true;

            auto window = get_window(handle);
            if (window) switch (message) {
            case WM_PAINT:
                window->render();

                glFlush();
                SwapBuffers(window->_drawing_context);
                return null;

            case WM_SIZE:
                window->_size = { LOWORD(l), HIWORD(l) };

                glViewport(null, null, window->_size.x, window->_size.y);
                PostMessageA(handle, WM_PAINT, null, null);
                return null;

            case WM_MOVE:
                window->_position = { LOWORD(l), HIWORD(l) };
                break;

            case WM_CLOSE:
                PostQuitMessage(null);
                break;

#ifdef NCORE_GWINDOW_UTILS
            case WM_KEYUP:
                if (w == NCORE_GWINDOW_RESET_KEY) {
                    *dialogs_count = null;
                }
                break;
#endif

            default: break;
            }

            return DefWindowProcA(handle, message, w, l);
        }

    public:
        static __forceinline auto __fastcall create(const std::string& title, const vec2i32& size, i32_t delay, callback_t render_callback = nullptr, callback_t close_callback = nullptr, void* callback_data = nullptr, const std::vector<hint_t>& hints = std::vector<hint_t>()) noexcept {
            auto window = new gwindow();

            window->_size = size;
            auto position = window->_position = { NCORE_GWINDOW_DEFAULT_POSITION };

            if (glwindow::create(window, event_handler, title, position, size, nullptr, hints)) {
                window->_delay = delay;

                window->_render_callback = render_callback;
                window->_close_callback = close_callback;
                window->_callback_data = callback_data;

                register_window(window);
            }
            else {
                delete window;
                window = nullptr;
            }

            return window;
        }

        static __forceinline auto __fastcall create_async(gwindow** _window, const std::string& title, const vec2i32& size, i32_t delay, callback_t render_callback = nullptr, callback_t open_callback = nullptr, callback_t close_callback = nullptr, void* callback_data = nullptr, const std::vector<hint_t>& hints = std::vector<hint_t>()) noexcept {
            auto data = creation_data_t{
                _window,
                title,
                size,
                delay,
                render_callback,
                open_callback,
                close_callback,
                callback_data,
                hints,
                -1
            };
            
            auto thread = thread::create(handling_thread, &data);

            while (data.result == -1) {
                thread::sleep(100);
            }

            if (data.result == 1) return thread;

            return ncore::thread();
        }

        __forceinline auto handle_events() noexcept {
            return glwindow::handle_events();
        }

        __forceinline auto terminate() noexcept {
            unregister_window(_handle);
            return glwindow::terminate();
        }

        __forceinline auto close() noexcept {
            return glwindow::close();
        }

        __forceinline auto move(long x, long y) noexcept {
            return glwindow::move(x, y);
        }

        __forceinline auto resize(long width, long height) noexcept {
            return glwindow::resize(width, height);
        }

        __forceinline auto& context() noexcept {
            return _context;
        }

        __forceinline auto context() const noexcept {
            return _context;
        }

        __forceinline constexpr auto& delay() noexcept {
            return _delay;
        }

        __forceinline constexpr auto delay() const noexcept {
            return _delay;
        }

        __forceinline auto size() const noexcept {
            return _size;
        }

        __forceinline auto position() const noexcept {
            return _position;
        }

    private:
        static __declspec(noinline) void handling_thread(creation_data_t* data) noexcept {
            auto info = *data;
            auto window = create(info.title, info.size, info.delay, info.render_callback, info.close_callback, info.callback_data, info.hints);

            if (!window) {
                data->result = false;
                return;
            }

            if (data->window) {
                *data->window = window;
            }

            if (info.open_callback) {
                info.open_callback(*window, info.callback_data);
            }

            data->result = true;

            window->handle_events();

            if (info.close_callback) {
                info.close_callback(*window, info.callback_data);
            }

            return window->terminate();
        }

    public:
        class gui {
        public:
            using window_t = ImGuiWindow;
            using context_t = ImGuiContext;

            using callback_t = get_procedure_t(void, , gwindow&, window_t&, void*);

            struct font_info {
                float size; //font size
                const void* data; //font data
            };

            static __forceinline auto initialize(gwindow& window, const std::vector<font_info>& fonts = std::vector<font_info>(), decltype(ImGui::StyleColorsLight)* style_set_procedure = nullptr, const char* ini_file_name = nullptr) noexcept {
                //IMGUI_CHECKVERSION();

                auto context = window.context() = ImGui::CreateContext();
                ImGui::SetCurrentContext(context);

                ImGui_ImplWin32_InitForOpenGL(window.handle());
                ImGui_ImplOpenGL3_Init("#version 130");

                if (!style_set_procedure) {
                    style_set_procedure = &ImGui::StyleColorsLight;
                }

                style_set_procedure(nullptr);

                auto io = &ImGui::GetIO();
                auto config = ImFontConfig();
                config.OversampleH = config.OversampleV = config.PixelSnapH = 1;

                for (auto& font : fonts) {
                    io->Fonts->AddFontFromMemoryTTF((void*)font.data, 1, font.size, &config, io->Fonts->GetGlyphRangesCyrillic());
                }

                io->IniFilename = ini_file_name;

                return context;
            }

            static __forceinline auto render(gwindow& window, const std::string& title, callback_t render_callback = nullptr, void* callback_data = nullptr, ImGuiWindowFlags flags = __defaultGuiFrameFlags) noexcept {
                ImGui::SetCurrentContext(window.context());

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(), ImGuiCond_Once);

                ImGui::SetNextWindowSize(ImVec2(float(window.size().x), float(window.size().y)));
                ImGui::Begin(title.c_str(), nullptr, flags);

                if (render_callback) {
                    render_callback(window, *ImGui::GetCurrentWindow(), callback_data);
                }

                ImGui::End();

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
        };

        struct configuration {
            std::string title;
            ncore::vec2i32 size;
            i32_t delay;
            std::vector<gwindow::hint_t> hints;

            struct {
                gui::font_info font;

                ui32_t flags;
                gui::callback_t render_callback;
                void* callback_data;
            }gui;

            __forceinline configuration() = default;

            __forceinline __fastcall configuration(const std::string& title, const ncore::vec2i32& size, i32_t delay,
                gui::callback_t gui_render_callback = nullptr, void* gui_callback_data = nullptr, 
                ui32_t gui_flags = null, float gui_font_size = null, const void* gui_font_data = nullptr,
                const std::vector<gwindow::hint_t>& hints = std::vector<gwindow::hint_t>()) noexcept {
                this->title = title;
                this->size = size;
                this->delay = delay;
                this->hints = hints;

                this->gui.font.size = gui_font_size;
                this->gui.font.data = gui_font_data;
                this->gui.flags = gui_flags;
                this->gui.render_callback = gui_render_callback;
                this->gui.callback_data = gui_callback_data;
            }

            template<typename _t = void> __forceinline constexpr auto callback_data() const noexcept {
                return (_t*)gui.callback_data;
            }
        };
    };

#ifdef NCORE_GWINDOW_UTILS
    using gwindow_callback_t = get_procedure_t(void, , gwindow&, gwindow::configuration*);

    extern gwindow* primary_window; //your primary window
    extern const gwindow::gui::font_info default_font; //default font info
    
    //todo:
    //static __forceinline void get_foreground_window_properties(vec2i32* _size, vec2i32* _pos) noexcept {
    //    auto foreground = GetForegroundWindow();
    //    if (!foreground) return;
    //
    //
    //}

    static void gwindow_open_event(gwindow& window, gwindow::configuration* data) noexcept {
        if (window.handle() != primary_window->handle()) {
            auto position = primary_window->position() + (primary_window->size() / 2) - (data->size / 2);
            window.move(position.x, position.y);
        }

        gwindow::gui::initialize(window, { data->gui.font });
    }

    static void gwindow_render_event(gwindow& window, gwindow::configuration* data) noexcept {
        return gwindow::gui::render(window, data->title, 
            data->gui.render_callback, data->gui.callback_data,
            data->gui.flags ? data->gui.flags : ncore::__defaultGuiFrameFlags);
    }

    static void gwindow_close_event(gwindow& window, gwindow::configuration* data) noexcept {
        if (!data) return;

        if (data->gui.callback_data) {
            delete data->gui.callback_data;
        }

        return delete data;
    }

    static __forceinline auto __fastcall create_gwindow_async(const gwindow::configuration& configuration, gwindow** _window = nullptr,
        const std::vector<gwindow::hint_t>& hints = std::vector<gwindow::hint_t>(),
        gwindow_callback_t render_callback = nullptr,
        gwindow_callback_t open_callback = nullptr,
        gwindow_callback_t close_callback = nullptr,
        void* gui_callback_data = nullptr) noexcept {

        auto data = new gwindow::configuration(configuration);
        if (gui_callback_data) {
            data->gui.callback_data = gui_callback_data;
        }

        if (!data->gui.font.data) {
            data->gui.font.data = default_font.data;
        }

        if (!data->gui.font.size) {
            data->gui.font.size = default_font.size;
        }

        return gwindow::create_async(_window, configuration.title,
            { configuration.size.x, configuration.size.y }, configuration.delay,
            gwindow::callback_t(render_callback ? render_callback : gwindow_render_event),
            gwindow::callback_t(open_callback ? open_callback : gwindow_open_event),
            gwindow::callback_t(close_callback ? close_callback : gwindow_close_event),
            data, hints.empty() ? configuration.hints : hints);
    }

    //horizontal tab
    class htab {
    public:
        using collection = std::vector<htab>;

        using render_callback_t = get_procedure_t(void, , gwindow::gui::window_t&);
        using context_callback_t = get_procedure_t(void, , const htab&);

    protected:
        std::string _label;

        render_callback_t _render_callback;
        context_callback_t _context_callback;
        collection _childs;
        htab* _selected;

    public:
        __forceinline __fastcall htab(const std::string& label, render_callback_t render_callback, context_callback_t context_callback = nullptr, const collection& childs = collection()) noexcept {
            _label = label;
            _render_callback = render_callback;
            _context_callback = context_callback;
            _childs = childs;
            _selected = childs.empty() ? nullptr : &_childs.front();
        }

        __forceinline __fastcall htab(const collection& childs) noexcept {
            _childs = childs;
            _selected = childs.empty() ? nullptr : &_childs.front();
        }

        __forceinline constexpr auto label() const noexcept {
            return _label;
        }

        __forceinline constexpr auto render_callback() const noexcept {
            return _render_callback;
        }

        __forceinline void render_head() noexcept {
            if (_childs.empty()) return;
            
            const auto& style = ImGui::GetStyle();
            const auto font = ImGui::GetFont();

            constexpr const auto push_style = []() noexcept {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1.f, 1.f });
                ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, { .5f, .5f });
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 1.f, 1.f });
                };

            constexpr const auto pop_style = []() noexcept {
                ImGui::PopStyleVar(3);
                };

            push_style();
            ImGui::BeginChild(ncore::format_string("##htab_head_%llx", this).c_str(), { 0.f, float(font->FontSize) + style.WindowPadding.y * 2 }, true, ImGuiWindowFlags_NoBringToFrontOnFocus);

            const auto window = ImGui::GetCurrentWindow();
            const auto tab_width = (window->Size.x - (style.WindowPadding.x * 2 + style.ItemSpacing.x * (_childs.size() - 1))) / _childs.size();

            for (index_t i = 0; i < _childs.size(); i++) {
                auto& child = _childs[i];

                if (i) {
                    ImGui::SameLine();
                }

                if (ImGui::Selectable(child._label.c_str(), child._render_callback == _selected->_render_callback, ImGuiSelectableFlags_None, { tab_width, 0.f })) {
                    _selected = &child;
                }

                if (child._context_callback) {
                    pop_style();

                    if (ImGui::BeginPopupContextItem(ncore::format_string("##htab_context_%llx", &child).c_str())) {
                        child._context_callback(child);

                        ImGui::EndPopup();
                    }

                    push_style();
                }
            }

            ImGui::EndChild();

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.ItemSpacing.y);

            return pop_style();
        }

        __forceinline void render() noexcept {
            render_head();

            if (_selected) return _selected->render();

            if (_render_callback) {
                ImGui::BeginChild(("##htab_frame_" + _label).c_str(), { 0.f, 0.f }, true, ImGuiWindowFlags_NoBringToFrontOnFocus);
                _render_callback(*ImGui::GetCurrentWindow());
                ImGui::EndChild();
            }
            else return ImGui::CenteredText("Unimplemented yet.");
        }

        __forceinline htab* select_by_callback(render_callback_t render_callback) noexcept {
            for (auto& child : _childs) {
                if (child._render_callback != render_callback) continue;

                return _selected = &child;
            }

            return nullptr;
        }
    };

    //quick dialog
    class qdialog {
    public:
        struct window_inner_info {
            float inner_width;
            float half_width;
            float quad_width;
            float answers_width;

            float area_height;
        };

        using render_callback_t = get_procedure_t(void, , gwindow&, gwindow::gui::window_t&, const qdialog&, const window_inner_info&);
        using window_callback_t = get_procedure_t(void, , gwindow&, const qdialog&);

    private:
        struct {
            bool* opened;
            sindex_t* result;
        }_out;

        std::vector<std::string> _answers;
        bool _answers_on_bottom;

        render_callback_t _render_callback;
        window_callback_t _open_callback;
        window_callback_t _close_callback;
        void* _callback_data;

        bool _overlap_main;
        bool _wrap_frame;
        sindex_t _answer;

        __forceinline __fastcall qdialog(const std::vector<std::string>& answers, bool bottom, render_callback_t render_callback, window_callback_t open_callback, window_callback_t close_callback, void* data, bool* opened, sindex_t* result, bool wrap_frame, bool overlap_main) noexcept {
            _answer = -1;

            _out.opened = opened;
            _out.result = result;
            _answers = answers;
            _answers_on_bottom = bottom;
            _render_callback = render_callback;
            _close_callback = close_callback;
            _callback_data = data;
            _wrap_frame = wrap_frame;
            _overlap_main = overlap_main;
        }

        __forceinline void render(gwindow& window, gwindow::gui::window_t& gui) noexcept {
            const auto font = ImGui::GetFont();
            const auto& style = ImGui::GetStyle();

            auto inner_info = window_inner_info();
            inner_info.inner_width = gui.Size.x - style.WindowPadding.x * 2;
            inner_info.answers_width = _answers.empty() ? 0.f : (inner_info.inner_width - style.ItemSpacing.x * (_answers.size() - 1)) / _answers.size();

            const auto render_answers = [&]() {
                for (count_t i = 0, j = _answers.size(); i < j; i++) {
                    if (i) {
                        ImGui::SameLine();
                    }

                    if (ImGui::Button(_answers[i].c_str(), { inner_info.answers_width, 0.f })) {
                        _answer = i;

                        window.close();
                    }
                }
                };

            if (!_answers_on_bottom) {
                render_answers();
            }

            if (_render_callback) {
                inner_info.half_width = (inner_info.inner_width - style.ItemSpacing.x * 1) * .5f;
                inner_info.quad_width = (inner_info.inner_width - style.ItemSpacing.x * 2) * .25f;

                inner_info.area_height = gui.Size.y - ((font->FontSize + style.ItemSpacing.y + style.WindowPadding.y * 2 + style.FramePadding.y * 2) * !_answers.empty());

                if (_wrap_frame) {
                    ImGui::BeginChild("##qdialog_frame", { 0.f, inner_info.area_height }, false, ImGuiWindowFlags_NoBringToFrontOnFocus);
                }

                _render_callback(window, gui, *this, inner_info);

                if (_wrap_frame) {
                    ImGui::EndChild();
                }
            }
            else {
                ImGui::CenteredText("Hello!");
            }

            if (_answers_on_bottom) {
                render_answers();
            }
        }

        static void gui_frame(gwindow& gwindow, gwindow::gui::window_t& gui, qdialog* data) noexcept {
            return data->render(gwindow, gui);
        }

        static void window_render_event(gwindow& window, gwindow::configuration* data) noexcept {
            return gwindow_render_event(window, data);
        }

        static void window_open_event(gwindow& window, gwindow::configuration* data) noexcept {
            auto dialog = data->callback_data<qdialog>();

            if (dialog->_open_callback) {
                dialog->_open_callback(window, *dialog);
            }

            if (dialog->_out.opened) {
                *dialog->_out.opened = true;
            }

            if (dialog->_overlap_main) {
                (*dialogs_count)++;
            }

            return gwindow_open_event(window, data);
        }

        static void window_close_event(gwindow& window, gwindow::configuration* data) noexcept {
            auto dialog = data->callback_data<qdialog>();

            if (dialog->_overlap_main) {
                (*dialogs_count)--;
            }

            if (dialog->_out.result) {
                *dialog->_out.result = dialog->_answer;
            }

            if (dialog->_close_callback) {
                dialog->_close_callback(window, *dialog);
            }

            if (dialog->_callback_data) {
                delete dialog->_callback_data;
            }

            if (dialog->_out.opened) {
                *dialog->_out.opened = false;
            }

            return gwindow_close_event(window, data);
        }

    public:
        static __forceinline auto __fastcall open(
            qdialog::render_callback_t render_callback, qdialog::window_callback_t open_callback,
            qdialog::window_callback_t close_callback, void* callback_data,
            const std::string& title, const ncore::vec2i32& size,
            const std::vector<std::string>& answers, bool bottom = true,
            sindex_t* _result = nullptr, bool* _opened = nullptr, gwindow** _window = nullptr,
            bool resizable = false, bool top_most = true, bool wrap_frame = true, bool overlap_main = true) noexcept {

            auto configuration = gwindow::configuration(title, size,
                __dialogWindowDelay, gwindow::gui::callback_t(gui_frame), 
                new qdialog(answers, bottom, render_callback, 
                    open_callback, close_callback, callback_data, 
                    _opened, _result, wrap_frame, overlap_main));

            return create_gwindow_async(configuration, _window,
                { {gwindow::hint_t::resizable, resizable}, {gwindow::hint_t::top_most, top_most} },
                window_render_event,
                window_open_event,
                window_close_event);
        }

        template<typename _t> __forceinline constexpr auto callback_data() const noexcept {
            return (_t*)this->_callback_data;
        }

        __forceinline constexpr auto result() const noexcept {
            return _answer;
        }
    };

    //question message
    class qmessage {
    private:
        std::string _message;
        ui32_t _color;
        bool _centred;

        __forceinline __fastcall qmessage(const std::string& message, ui32_t color, bool centred) noexcept {
            _message = message;
            _color = color;
            _centred = centred;
        }

        __forceinline void render(gwindow::gui::window_t& gui) noexcept {
            if (_color) {
                ImGui::PushStyleColor(ImGuiCol_Text, _color);
            }

            if (_centred) {
                ImGui::CenteredText(_message.c_str());
            }
            else {
                ImGui::Text(_message.c_str());
            }

            if (_color) {
                ImGui::PopStyleColor();
            }
        }

        static void frame(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            return dialog.callback_data<qmessage>()->render(gui);
        }

        static void open(gwindow& window, const qdialog& dialog) noexcept {
            MessageBeep(null);
        }

    public:
        static __forceinline auto __fastcall open(const std::string& message, const std::vector<std::string>& answers, sindex_t* _result = nullptr, bool* _opened = nullptr, ui32_t text_color = null, bool text_centred = false, gwindow** _window = nullptr) noexcept {
            return qdialog::open(frame, open, nullptr, new qmessage(message, text_color, text_centred), "Notification", { 450, 150 }, answers, true, _result, _opened, _window);
        }
    };

    //file selector
    class fselector {
    public:
        using apply_callback_t = get_procedure_t(bool, , const std::string&, void*);

    private:
        bool _initialize;
        ImGuiFileDialog _selector;
        std::string _extensions;

        apply_callback_t _apply_callback;
        void* _callback_data;

        __forceinline __fastcall fselector(apply_callback_t apply_callback, void* callback_data, const std::string& extensions) noexcept {
            _initialize = true;
            _extensions = extensions;

            _apply_callback = apply_callback;
            _callback_data = callback_data;
        }

        __forceinline void render(gwindow& window, gwindow::gui::window_t& gui) noexcept {
            if (_initialize) {
                _initialize = false;

                _selector.OpenDialog("##file_selector_dialog", "##file_selector_title", _extensions.c_str(), ".", 1, nullptr, ImGuiFileDialogFlags_DontShowHiddenFiles);

                ImGui::SetNextWindowPos({ 0.f,0.f });
            }

            if (!_selector.Display("##file_selector_dialog", ncore::__defaultGuiFrameFlags, gui.Size, gui.Size)) return;

            auto apply = _selector.IsOk();
            auto path = _selector.GetFilePathName();

            _selector.Close();

            if (apply && _apply_callback) {
                apply = _apply_callback(path, _callback_data);

                _selector = decltype(_selector)();

                if (!apply) {
                    _initialize = true;
                    return;
                }
            }

            return window.close();
        }

        static void frame(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            return dialog.callback_data<fselector>()->render(window, gui);
        }

    public:
        static __forceinline auto __fastcall open(apply_callback_t apply_callback, void* callback_data, const std::string& extensions = ".*", bool* _opened = nullptr, gwindow** _window = nullptr) noexcept {
            return qdialog::open(frame, nullptr, nullptr, new fselector(apply_callback, callback_data, extensions), "Select file", { 600, 375 }, { }, true, nullptr, _opened, _window, true, false, false);
        }
    };

    //process selector
    class pselector {
    public:
        using apply_callback_t = get_procedure_t(bool, , const ncore::process&, void*);

    private:
        apply_callback_t _apply_callback;
        void* _callback_data;

        __forceinline __fastcall pselector(apply_callback_t apply_callback, void* callback_data) noexcept {
            _apply_callback = apply_callback;
            _callback_data = callback_data;
        }

        __forceinline void render(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            static auto processes = new std::vector<ncore::process>();
            static auto last_refresh_time = new ncore::time();
            static auto target = (ncore::process*)nullptr;
            static auto search_buffer = new char[0xff] {'\0'};

            ImGui::SetNextItemWidth(inner.half_width);
            ImGui::InputText("##target_name", search_buffer, 0xff);
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Process name");
            }

            ImGui::SameLine();

            if (ImGui::Button("Refresh", { inner.quad_width, 0.f }) || !last_refresh_time->year()) {
                *last_refresh_time = ncore::time::current();
                *processes = ncore::get_processes();

                for (count_t i = 0; i < processes->size(); i++) {
                    auto& process = processes->at(i);
                    if (process.search_module("jvm.dll")) continue; //todo: make it burn them

                    processes->erase(processes->begin() + i);
                    i--;
                }
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%02d:%02d:%02d", last_refresh_time->hour(), last_refresh_time->minute(), last_refresh_time->secound());
            }

            ImGui::SameLine();

            ImGui::BeginDisabled(!target || !_apply_callback);
            if (ImGui::Button("Attach", { inner.quad_width, 0.f })) {
                if (_apply_callback(*target, _callback_data)) {
                    window.close();
                }
            }
            ImGui::EndDisabled();

            if (ImGui::BeginChild("##process_list_frame", ImVec2(), true)) {
                if (processes->empty()) {
                    ImGui::CenteredText("No java processes launched!"); //todo: too
                }
                else for (auto& process : *processes) {
                    if (!process.alive()) continue;

                    auto process_name = process.get_name();

                    if (strlen(search_buffer)) {
                        if (ncore::string_to_lower(process_name).find(ncore::string_to_lower(search_buffer)) != 0) continue;
                    }

                    if (!ImGui::IsNextItemVisible()) {
                        auto font_size = ImGui::GetFont()->FontSize;
                        ImGui::Dummy({ font_size, font_size });
                        continue;
                    }

                    auto label = ncore::format_string("[%d] %s\n", process.id(), process_name.c_str());
                    if (ImGui::Selectable(label.c_str(), target ? target->id() == process.id() : false)) {
                        if (target) {
                            delete target;
                        }

                        target = new ncore::process(process);
                    }

                    if (ImGui::IsItemHovered()) {
                        const auto path_length_limit = ((int(gui.Size.x) / 5) * 4) / int(ImGui::GetFont()->FontSize) - 3;

                        auto tooltip = process.get_path();
                        tooltip.resize(path_length_limit);
                        tooltip += "...";

                        auto process_windows = process.get_windows();
                        if (!process_windows.empty()) {
                            auto titles = std::string();

                            for (auto window : process_windows) {
                                auto title = window.get_title();
                                titles += ncore::format_string("%c %#llx%s\n",
                                    window.visible() ? (window.focused() ? '*' : '+') : '-',
                                    window.handle,
                                    title.empty() ? "" : (" - [" + title + "]").c_str());
                            }

                            tooltip += "\n" + titles;
                        }

                        ImGui::SetTooltip(tooltip.c_str());
                    }
                }

                ImGui::EndChild();
            }
        }

        static void frame(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            return dialog.callback_data<pselector>()->render(window, gui, dialog, inner);
        }

    public:
        static __forceinline auto __fastcall open(apply_callback_t apply_callback, void* callback_data, bool* _opened = nullptr, gwindow** _window = nullptr) noexcept {
            return qdialog::open(frame, nullptr, nullptr, new pselector(apply_callback, callback_data), "Attach process", { 500, 300 }, { }, true, nullptr, _opened, _window, false, false, false);
        }
    };

    //results shower
    class rshower {
    public:
        using context_callback_t = get_procedure_t(void, , gwindow&, const std::string&, index_t, void*);

    private:
        std::string _info;
        std::vector<std::string> _rows;

        context_callback_t _context_callback;
        void* _callback_data;
        bool _release_callback_data;

        __forceinline rshower(context_callback_t context_callback, void* callback_data, bool release_callback_data, const std::vector<std::string>& results, const std::string& info) noexcept {
            _info = info.empty() ? ncore::format_string("%d elements", results.size()) : info;
            _rows = results;
            _context_callback = context_callback;
            _callback_data = callback_data;
            _release_callback_data = release_callback_data;
        }

        void render(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            ImGui::Selectable(_info.c_str());

            if (ImGui::BeginPopupContextItem("##rshower_context")) {
                if (ImGui::MenuItem("Copy to clipboard")) {
                    auto data = std::string();
                    for (auto& row : _rows) {
                        data += row + "\n";
                    }

                    ImGui::SetClipboardText(data.c_str());
                    MessageBeep(null);
                }

                if (ImGui::MenuItem("Save to file")) {
                    auto data = std::string();
                    for (auto& row : _rows) {
                        data += row + "\n";
                    }

                    auto time = ncore::time::current();
                    auto path = ncore::process::current().get_directory() + ncore::format_string("jha.results.%02d%02d%02d.txt", time.hour(), time.minute(), time.secound());
                    if (ncore::write_file(path, data.data(), data.size())) {
                        qmessage::open("Successfully saved to\n" + path, { "Ok" });
                    }
                    else {
                        qmessage::open("Can't save to\n" + path, { "Ok" });
                    }
                }

                ImGui::EndPopup();
            }

            if (ImGui::BeginChild("##rshower_frame", {}, true, ImGuiWindowFlags_NoBringToFrontOnFocus)) {
                for (auto i = index_t(), j = _rows.size(); i < j; i++) {
                    if (!ImGui::IsNextItemVisible()) {
                        auto font_size = ImGui::GetFont()->FontSize;
                        ImGui::Dummy({ font_size, font_size });
                        continue;
                    }

                    auto& row = _rows[i];
                    ImGui::Selectable(row.c_str());

                    if (!ImGui::BeginPopupContextItem((row + "_" + ncore::format_string("%llx", i)).c_str())) continue;

                    if (ImGui::MenuItem("Copy")) {
                        ImGui::SetClipboardText(row.c_str());
                    }

                    if (_context_callback) {
                        _context_callback(window, row, i, _callback_data);
                    }

                    ImGui::EndPopup();
                }

                ImGui::EndChild();
            }
        }

        static void frame(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            return dialog.callback_data<rshower>()->render(window, gui, dialog, inner);
        }

        static void close(gwindow& window, const qdialog& dialog) noexcept {
            auto instance = dialog.callback_data<rshower>();

            if (instance->_callback_data && instance->_release_callback_data) {
                delete instance->_callback_data;
            }
        }

    public:
        static __forceinline auto __fastcall open(const char* title, context_callback_t context_callback, void* callback_data, bool release_callback_data, const std::vector<std::string>& rows, const std::string& info = std::string(), bool* _opened = nullptr, gwindow** _window = nullptr) noexcept {
            return qdialog::open(frame, nullptr, close, new rshower(context_callback, callback_data, release_callback_data, rows, info), title, { 300, 175 }, { "Cancel" }, true, nullptr, _opened, _window, true, true, true, false);
        }
    };

    //progress bar
    class pbar {
    private:
        bool _release_description;
        char _chars_count;

        const std::string* _description;
        float* _progress;

        struct {
            ui64_t previous, current;
        }_time;

        __forceinline pbar(const std::string& description, float* progress) noexcept {
            _release_description = true;
            _description = new std::string(description);
            _progress = progress;
        }

        __forceinline pbar(const std::string* description, float* progress) noexcept {
            _release_description = false;
            _description = description;
            _progress = progress;
        }

        void render(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            if (!_description->empty()) {
                ImGui::Text(_description->c_str());
            }

            auto progress = float(.5f);
            auto text = std::string();

            if (_progress) {
                progress = *_progress;
            }
            else {
                if ((_time.current = GetTickCount64()) >= (_time.previous + __pbarAnimationDelay)) {
                    _time.previous = _time.current;

                    if (_chars_count++ >= __pbarAnimationCharsCount) {
                        _chars_count = 0;
                    }
                }

                text = __pbarAnimationChar;
                for (auto i = char(); i < _chars_count; i++) {
                    text += " " + std::string(__pbarAnimationChar);
                }
            }

            ImGui::Spacing();
            ImGui::ProgressBar(progress, { -FLT_MIN, 0.f }, text.empty() ? nullptr : text.c_str());
        }

        static void frame(gwindow& window, gwindow::gui::window_t& gui, const qdialog& dialog, const qdialog::window_inner_info& inner) noexcept {
            return dialog.callback_data<pbar>()->render(window, gui, dialog, inner);
        }

        static void close(gwindow& window, const qdialog& dialog) noexcept {
            auto instance = dialog.callback_data<pbar>();

            if (instance->_release_description) {
                delete instance->_description;
            }
        }

    public:
        static __forceinline auto __fastcall open(const std::string& description, float* progress, bool* _opened = nullptr, gwindow** _window = nullptr) noexcept {
            return qdialog::open(frame, nullptr, nullptr, new pbar(description, progress), "Progress", { 450, 125 }, { "Cancel" }, true, nullptr, _opened, _window, false, true, true, true);
        }

        static __forceinline auto __fastcall open(const std::string* description, float* progress, bool* _opened = nullptr, gwindow** _window = nullptr) noexcept {
            return qdialog::open(frame, nullptr, nullptr, new pbar(description, progress), "Progress", { 450, 125 }, { "Cancel" }, true, nullptr, _opened, _window, false, true, true, true);
        }
    };
#endif
}