#pragma once
#include "thread.hpp"

//to work with multiply threads, you need this in imconfig.h:
//  struct ImGuiContext;
//  extern thread_local ImGuiContext* ImGuiTLContext;
//  #define GImGui ImGuiTLContext
//and this in one of your .cpp files:
//  thread_local ImGuiContext* ImGuiTLContext;

#include "includes/gwindow/glfw/glfw3.h"
#include "includes/gwindow/glfw/glfw3native.h"

#include "includes/gwindow/imgui/imgui.h"
#include "includes/gwindow/imgui/imgui_internal.h"
#include "includes/gwindow/imgui/backends/imgui_impl_glfw.h"
#include "includes/gwindow/imgui/backends/imgui_impl_opengl3.h"

namespace ncore {
    class gwindow {
    public:
        using callback_t = get_procedure_t(void, , gwindow&, void*);

    private:
        class core {
        public:
            struct hint_t {
                int name, value;
            };

            static __forceinline auto initialize() noexcept {
                return glfwInit();
            }

            static __forceinline auto terminate() noexcept {
                return glfwTerminate();
            }

            static __forceinline auto create_window(const std::string& title, long width, long height, const std::vector<hint_t>& hints = std::vector<hint_t>()) noexcept {
                static auto initialized = false;
                
                if (!initialized) {
                    if (!(initialized = initialize())) return (GLFWwindow*)nullptr;
                }

                for (auto& hint : hints) {
                    glfwWindowHint(hint.name, hint.value);
                }

                auto result = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
                if (result) {
                    glfwMakeContextCurrent(result);
                    glfwSwapInterval(GLFW_DONT_CARE);

                    glClearColor(null, null, null, null);
                }

                return result;
            }

            static __forceinline auto terminate_window(GLFWwindow* window) noexcept {
                if (window) {
                    glfwDestroyWindow(window);
                }
            }

            static __forceinline auto update_window_frame(GLFWwindow* window, i32_t delay) noexcept {
                if (!window) return;

                glfwSwapBuffers(window);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

                return ncore::thread::sleep(delay);
            }

            static __forceinline auto is_window_alive(GLFWwindow* window) noexcept {
                return !glfwWindowShouldClose(window);
            }

            static __forceinline auto proceed_window_events() noexcept {
                return glfwPollEvents();
            }

            static __forceinline auto get_window_size(GLFWwindow* window) noexcept {
                struct size {
                    long width, height;
                }result;

                glfwGetWindowSize(window, (int*)&result.width, (int*)&result.height);

                return result;
            }

            static __forceinline auto get_window_position(GLFWwindow* window) noexcept {
                struct position {
                    long x, y;
                }result;

                glfwGetWindowPos(window, (int*)&result.x, (int*)&result.y);

                return result;
            }

            static __forceinline auto set_current(GLFWwindow* window) noexcept {
                return glfwMakeContextCurrent(window);
            }
        };

        struct render_thread_data {
            gwindow* window;
            callback_t create_callback;
            std::string title;
            long width, height;
            std::vector<core::hint_t> hints;
        };

        GLFWwindow* _handle;
        void* _context;
        i32_t _delay;
        callback_t _render_callback;
        callback_t _close_callback;
        void* _callback_data;

        __forceinline constexpr gwindow(GLFWwindow* handle, void* context, i32_t delay, callback_t render_callback, callback_t close_callback, void* callback_data) noexcept {
            _handle = handle;
            _context = context;
            _delay = delay;
            _render_callback = render_callback;
            _close_callback = close_callback;
            _callback_data = callback_data;
        }

    public:
        __forceinline gwindow() = default;

        static __forceinline auto __fastcall create_for_this(const std::string& title, long width, long height, i32_t delay, callback_t render_callback = nullptr, callback_t close_callback = nullptr, void* callback_data = nullptr, const std::vector<core::hint_t>& hints = std::vector<core::hint_t>()) noexcept {
            auto handle = core::create_window(title, width, height, hints);
            return gwindow(handle, nullptr, delay, render_callback, close_callback, callback_data);
        }

        static __forceinline auto& __fastcall create_async(const std::string& title, long width, long height, i32_t delay, callback_t render_callback = nullptr, callback_t create_callback = nullptr, callback_t close_callback = nullptr, void* callback_data = nullptr, const std::vector<core::hint_t>& hints = std::vector<core::hint_t>(), ncore::thread* _thread = nullptr) noexcept {
            auto window = gwindow(nullptr, nullptr, delay, render_callback, close_callback, callback_data);

            auto thread_data = render_thread_data{
                &window,
                create_callback,
                title,
                width, height,
                hints
            };

            auto thread = ncore::thread::create(render_procedure, &thread_data);
            while (thread_data.window == &window) {
                ncore::thread::sleep(50);
            }

            if (_thread) {
                *_thread = thread;
            }

            return *thread_data.window;
        }

        __forceinline auto terminate() noexcept {
            auto handle = _handle;
            _handle = nullptr;
            return core::terminate_window(handle);
        }

        __forceinline constexpr auto valid() const noexcept {
            return _handle;
        }

        __forceinline constexpr auto handle() const noexcept {
            return _handle;
        }

        template<typename _t = void> __forceinline auto& context() noexcept {
            auto context = ((_t**)&_context);
            return *context;
        }

        template<typename _t = void> __forceinline auto context() const noexcept {
            return (_t*)_context;
        }

        __forceinline constexpr auto& delay() noexcept {
            return _delay;
        }

        __forceinline constexpr auto delay() const noexcept {
            return _delay;
        }

        __forceinline auto size() const noexcept {
            return core::get_window_size(_handle);
        }

        __forceinline auto position() const noexcept {
            return core::get_window_position(_handle);
        }

        __forceinline constexpr auto& render_callback() noexcept {
            return _render_callback;
        }

        __forceinline constexpr auto render_callback() const noexcept {
            return _render_callback;
        }

        __forceinline auto alive() const noexcept {
            return core::is_window_alive(_handle);
        }

        __forceinline auto begin_frame() noexcept {
            core::proceed_window_events();
            return core::is_window_alive(_handle);
        }

        __forceinline auto end_frame() noexcept {
            return core::update_window_frame(_handle, _delay);
        }

    private:
        static __declspec(noinline) void render_procedure(render_thread_data& data) noexcept {
            auto window = *data.window;

            if (window._handle = core::create_window(data.title, data.width, data.height, data.hints)) if (data.create_callback) {
                data.create_callback(window, nullptr);
            }

            if (!(data.window = &window)->_handle) return;

            while (window.begin_frame()) {
                if (window._render_callback) {
                    window._render_callback(window, window._callback_data);
                }

                window.end_frame();
            }

            if (window._close_callback) {
                window._close_callback(window, window._callback_data);
            }
        }

    public:
        class gui {
        public:
            using window_t = ImGuiWindow;
            using context_t = ImGuiContext;
            using render_procedure_t = get_procedure_t(void, , window_t&);

            struct font_info {
                int size; //font size
                const void* data; //font data
            };

            static __forceinline auto initialize(gwindow& window, const std::vector<font_info>& fonts = std::vector<font_info>(), decltype(ImGui::StyleColorsLight)* style_set_procedure = nullptr, const char* ini_file_name = nullptr) noexcept {
                IMGUI_CHECKVERSION();

                auto context = window.context<context_t>() = ImGui::CreateContext();
                ImGui::SetCurrentContext(context);

                ImGui_ImplGlfw_InitForOpenGL(window.handle(), true);
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

            static __forceinline auto render(gwindow& window, const std::string& title, render_procedure_t render_callback, ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration) noexcept {
                ImGui::SetCurrentContext(window.context<context_t>());

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGui::SetNextWindowPos(ImVec2(), ImGuiCond_Once);
                ImGui::SetNextWindowSize(ImVec2(window.size().width, window.size().height));
                ImGui::Begin(title.c_str(), nullptr, flags);

                if (render_callback) {
                    render_callback(*ImGui::GetCurrentWindow());
                }

                ImGui::End();

                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
        };
    };
}
