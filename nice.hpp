#ifndef _NICE_HPP
#define _NICE_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <iterator>

// Main entry point. You must provide this.
extern void program();

// --- two phase construction pattern -------------------------------
template <class T, typename... A>
static T* create(A... args)
{
    T* ptr = new T(args...);
    ptr->create();
    return ptr;
}

// --- signals ------------------------------------------------------
namespace nice {
template <typename... Args>
class signal {
public:
    signal() : current_id_(0) {}
    template <typename T> int connect(T* inst, void (T::* func)(Args...)) {
        return connect([=](Args... args) {
            (inst->*func)(args...);
            });
    }

    template <typename T> int connect(T* inst, void (T::* func)(Args...) const) {
        return connect([=](Args... args) {
            (inst->*func)(args...);
            });
    }

    int connect(std::function<void(Args...)> const& slot) const {
        slots_.insert(std::make_pair(++current_id_, slot));
        return current_id_;
    }

    void disconnect(int id) const {
        slots_.erase(id);
    }

    void disconnect_all() const {
        slots_.clear();
    }

    void emit(Args... p) {
        for (auto const& it : slots_) {
            it.second(std::forward<Args>(p)...);
        }
    }
private:
    mutable std::map<int, std::function<void(Args...)>> slots_;
    mutable int current_id_;
};
} // using nice


#ifdef _WIN32

// --- includes -----------------------------------------------------
#include <windows.h>
#include <tchar.h>


// --- forward declarations -----------------------------------------
extern int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd);

namespace nice {
    
    // --- types ----------------------------------------------------
    typedef HINSTANCE app_id;
    typedef HWND wnd_id;
    typedef UINT msg_id;
    typedef WPARAM par1;
    typedef LPARAM par2;
    typedef LRESULT result;
    typedef LONG coord; 
    typedef BYTE byte;


    // --- primitive structures -------------------------------------
    struct rct {
        union { coord left; coord x; coord x1; };
        union { coord top; coord y;  coord y1; };
        union { coord right; coord x2; };
        union { coord bottom; coord y2; };
    };

    struct pt {
        union { coord left; coord x; };
        union { coord top; coord y; };
    };

    struct size {
        union { coord width; coord w; };
        union { coord height; coord h; };
    };

    struct color {
        byte r;
        byte g;
        byte b;
        byte a;
    };

    // --- painter --------------------------------------------------
    class artist {
    public:
        artist(HDC hdc) {
            hdc_ = hdc;
        }

        // Method(s).
        void draw_rect(color c, const rct& r) {
            RECT rect = { r.left, r.top, r.right, r.bottom};
            HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
            ::FrameRect(hdc_, &rect, brush);
            ::DeleteObject(brush);
        }

    private:
        HDC hdc_; // Windows device context;
    };


    // --- window base ----------------------------------------------
    class native_wnd {
    public:
        native_wnd() : hwnd(0) {}
    protected:
        // Native window handle.
        HWND hwnd;

        // Member callback, must implement!
        virtual result local_wnd_proc(msg_id id, par1 p1, par2 p2) = 0;

        // Generic callback. Calls member callback.
        static LRESULT CALLBACK global_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            // Is it the very first message? Only on WM_NCCREATE.
            // TODO: Why does Windows 10 send WM_GETMINMAXINFO first?!
            native_wnd* self = nullptr;
            if (message == WM_NCCREATE) {
                LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
                auto self = static_cast<native_wnd*>(lpcs->lpCreateParams);
                self->hwnd = hWnd; // save the window handle too!
                SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            else
                self = reinterpret_cast<native_wnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

            // Chain...
            if (self != nullptr)
                return (self->local_wnd_proc(message, wParam, lParam));
            else
                return ::DefWindowProc(hWnd, message, wParam, lParam);
        }
    };

    template<class T>
    class wnd : public native_wnd
    {
    public:
        // Ctor.
        wnd() : native_wnd() {}

        // Method(s)
        virtual T* create() = 0;
        virtual void destroy() = 0;

        // Properties.
        virtual wnd_id id() { return hwnd; }
        virtual T* id(wnd_id id) { hwnd = id;  return (T*)this; }

        virtual T* text(std::string txt) {
            ::SetWindowText(id(), txt.data());
            return (T*)this;
        }

        virtual std::string text() {
            TCHAR sz[1024];
            GetWindowText(id(), sz, 1024);
            return sz;
        }

        // Events.
        signal<> created;        // Window created.
        signal<> destroyed;        // Window destroyed.
        signal<std::shared_ptr<artist>> paint;

    protected:
        // Overrides
        virtual result local_wnd_proc(msg_id id, par1 p1, par2 p2);

        // Properties.
        std::string text_;
    };

    
    template<class T>
    result wnd<T>::local_wnd_proc(msg_id id, par1 p1, par2 p2) {
        switch (id)
        {
        case WM_CREATE:
            created.emit();
            break;

        case WM_DESTROY:
            destroyed.emit();
            PostQuitMessage(0);
            break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(this->id(), &ps);
            paint.emit(std::make_shared<artist>(hdc));
            EndPaint(this->id(), &ps);
        }
        break;

        default:
            return ::DefWindowProc(this->id(), id, p1, p2);
        }
        return 0;
    }

    // --- application window ---------------------------------------
    class app_wnd : public wnd<app_wnd>
    {
    public:
        app_wnd(std::string text) : wnd() { text_ = text; class_ = "APP_WND"; }
        virtual ~app_wnd() { }
        
        app_wnd* create();
        void destroy();

    private:
        std::string class_;
        WNDCLASSEX wcex_;
    };


    // --- button ---------------------------------------------------
    class button : public wnd<button> {
    public:
        button(wnd_id parent, std::string text, rct r) : wnd(), parent_(parent) { text_ = text; class_ = "BUTTON"; r_ = r; }
        button* create();
        void destroy();
    protected:
        wnd_id parent_;
        std::string class_;
        rct r_;
    };


    // --- application ----------------------------------------------
    class app
    {
    public:
        static app_id id();
        static void id(app_id id);
        static int ret_code();
        static void run(std::unique_ptr<app_wnd> w);

    private:
        static app_id id_;
        static int ret_code_;
    };

    app_id app::id_{ 0 };
    int app::ret_code_{ 0 };

    app_id app::id() { return app::id_; }
    void app::id(app_id id) { app::id_ = id; }
    int app::ret_code() { return app::ret_code_; }

    void app::run(std::unique_ptr<app_wnd> w) {

        // Show and update main window.
        ::ShowWindow(w->id(), SW_SHOWNORMAL);

        // Enter message loop.
        MSG msg;
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        // Finally, set the return code.
        app::ret_code_ = (int)msg.wParam;
    }


    // --- cross reference functions --------------------------------
    app_wnd* app_wnd::create() {

        // Register window.
        ::ZeroMemory(&wcex_, sizeof(WNDCLASSEX));
        wcex_.cbSize = sizeof(WNDCLASSEX);
        wcex_.lpfnWndProc = wnd::global_wnd_proc;
        wcex_.hInstance = app::id();
        wcex_.lpszClassName = class_.data();

        if (!::RegisterClassEx(&wcex_)) { } // TODO: exception.

        // Create it.
        hwnd = ::CreateWindowEx(
            0,
            class_.data(),
            text_.data(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            800, 600,
            NULL,
            NULL,
            app::id(),
            this);

        if (!hwnd) { } // TODO: exception.

        return this;
    }

    void app_wnd::destroy() {
        ::DestroyWindow(id());
    }
    
    button* button::create() {

        // Create it.
        hwnd = ::CreateWindow(
            class_.data(),
            text_.data(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            r_.x,
            r_.y,
            r_.x2 - r_.x1 + 1,
            r_.y2 - r_.y1 + 1,
            parent_,
            NULL,
            app::id(),
            this);

        if (!hwnd) {} // TODO: exception.

        return this;
    }

    void button::destroy() {
        ::DestroyWindow(id());
    }

} // namespace nice


// --- startup code ---------------------------------------------
int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nShowCmd)
{
    // Set application id.
    nice::app::id(hInstance);

    // Run program.
    program();

    // And return success;
    return nice::app::ret_code();
}


#endif // #ifdef _WIN32

#endif // #ifndef _NICE_HPP