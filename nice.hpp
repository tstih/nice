#ifndef _NICE_HPP
#define _NICE_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <iterator>

// Main entry point. You must provide this.
extern void program();


namespace nice {


    // --- nice exceptions ----------------------------------------------
    #define throw_ex(ex, what) \
        throw ex(what, __FILE__,__FUNCTION__,__LINE__);

    class nice_exception : public std::exception {
    public:
        nice_exception(
            std::string what, 
            std::string file=nullptr, 
            std::string func=nullptr, 
            int line=0) : what_(what), file_(file), func_(func), line_(line) {};
        std::string what() { return what_; }
    protected:
        std::string what_;
        std::string file_; // __FILE__
        std::string func_; // __FUNCTION__
        int line_; // __LINE__
    };

    // --- two phase construction pattern -------------------------------

    // The resource (that requires two phase construction).
    template <typename T>
    class resource {
    public:
        virtual void create() = 0;
        virtual void destroy() noexcept = 0; // Can happen in an exception!
    };

    // Concept.
    template <typename T>
    concept TP = requires(T t) { 
        
        { t.create() };
        { t.destroy() };      
    };

    // The global create function.
    template <TP T, typename... A>
    std::shared_ptr<T> create(A... args) requires TP<T>
    {
        // Create a shared pointer with a custom deleter.
        std::shared_ptr<T> ptr(new T(args...), [](T* p) { p->destroy();  delete p; });
        ptr->create();
        return ptr;
    }


    // --- signals ------------------------------------------------------
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

    // --- layout manager -------------------------------------------
    class layout {
    public:
        virtual void apply() {}
    };

    class no_layout : public layout {

    };

    // --- window base ----------------------------------------------
    class native_wnd {
    public:
        // Ctor.
        native_wnd() : hwnd_(NULL) {}

        // Window handle.
        HWND hwnd_;
    };

    class wnd : public native_wnd, resource<wnd>
    {
    public:

        // Ctor. Default is no layout manager!
        wnd() : lm_(std::make_shared<no_layout>()) {}

        // Properties.
        virtual wnd_id id() { return hwnd_; }
        virtual void id(wnd_id id) { hwnd_ = id;  }

        virtual void text(std::string txt) {
            ::SetWindowText(id(), txt.data());
        }

        virtual std::string text() {
            TCHAR sz[1024];
            GetWindowText(id(), sz, 1024);
            return sz;
        }

        // Events.
        signal<> created;        // Window created.
        signal<> destroyed;      // Window destroyed.
        signal<std::shared_ptr<artist>> paint;

    protected:
        
        // Current layout manager.
        std::shared_ptr<layout> lm_;

        // Properties.
        std::string text_;

        // Generic callback. Calls member callback.
        static LRESULT CALLBACK global_wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
        {
            // Is it the very first message? Only on WM_NCCREATE.
            // TODO: Why does Windows 10 send WM_GETMINMAXINFO first?!
            wnd* self = nullptr;
            if (message == WM_NCCREATE) {
                LPCREATESTRUCT lpcs = reinterpret_cast<LPCREATESTRUCT>(lParam);
                auto self = static_cast<wnd*>(lpcs->lpCreateParams);
                self->id(hWnd); // save the window handle too!
                SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
            }
            else
                self = reinterpret_cast<wnd*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

            // Chain...
            if (self != nullptr)
                return (self->local_wnd_proc(message, wParam, lParam));
            else
                return ::DefWindowProc(hWnd, message, wParam, lParam);
        }

        // Local callback. Specific to wnd.
        virtual result local_wnd_proc(msg_id id, par1 p1, par2 p2) {
            switch (id)
            {
            case WM_CREATE:
                created.emit();
                break;

            case WM_DESTROY:
                destroyed.emit();
                break;

            case WM_PAINT:
            {
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(this->id(), &ps);
                paint.emit(std::make_shared<artist>(hdc));
                EndPaint(this->id(), &ps);
            }
            break;

            case WM_SIZING:
            case WM_SIZE:
                if (lm_ != nullptr)
                    lm_->apply();
                // And fall through...
            default:
                return ::DefWindowProc(this->id(), id, p1, p2);
            }
            return 0;
        }
    };

    // --- application window ---------------------------------------
    class app_wnd : public wnd
    {
    public:
        app_wnd(std::string text) : wnd() { 
            text_ = text; 
            class_ = "APP_WND"; 
        }

        virtual void create();
        virtual void destroy() noexcept;

        void add(std::shared_ptr<wnd> child) {
            ::SetParent(child->id(), id());
        }

    protected:
        virtual result local_wnd_proc(msg_id id, par1 p1, par2 p2) {
            // Forward the message down the chain.
            auto r = wnd::local_wnd_proc(id, p1, p2);
            // If message is quit then post quit message...
            if (id == WM_DESTROY)
                ::PostQuitMessage(0);
            // And return the result.
            return r;
        }

    private:
        std::string class_;
        WNDCLASSEX wcex_;
    };


    // --- button ---------------------------------------------------
    class button : public wnd {
    public:
        button(std::string text, rct r) : wnd() { text_ = text; r_ = r; }

        virtual void create();
        virtual void destroy() noexcept;

    protected:
        rct r_;
    };


    // --- text edit ------------------------------------------------
    class text_edit : public wnd {
    public:
        text_edit(rct r) : wnd(), r_(r) {}

        virtual void create();
        virtual void destroy() noexcept;

    protected:
        rct r_;
    };


    // --- application ----------------------------------------------
    class app
    {
    public:
        static app_id id();
        static void id(app_id id);
        static int ret_code();
        static void run(std::shared_ptr<app_wnd> w);

    private:
        static app_id id_;
        static int ret_code_;
    };

    app_id app::id_{ 0 };
    int app::ret_code_{ 0 };

    app_id app::id() { return app::id_; }
    void app::id(app_id id) { app::id_ = id; }
    int app::ret_code() { return app::ret_code_; }

    void app::run(std::shared_ptr<app_wnd> w) {

        // Show and update main window.
        ::ShowWindow(w->hwnd_, SW_SHOWNORMAL);

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
    void app_wnd::create() {

        // Register window.
        ::ZeroMemory(&wcex_, sizeof(WNDCLASSEX));
        wcex_.cbSize = sizeof(WNDCLASSEX);
        wcex_.lpfnWndProc = wnd::global_wnd_proc;
        wcex_.hInstance = app::id();
        wcex_.lpszClassName = class_.data();

        if (!::RegisterClassEx(&wcex_)) throw_ex(nice_exception, "Unable to register application window.");

        // Create it.
        hwnd_ = ::CreateWindowEx(
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

        if (!hwnd_)
            throw_ex(nice_exception, "Unable to create application window.");
    }

    void app_wnd::destroy() noexcept {
        ::DestroyWindow(id());
    }
    
    void button::create() {

        // Create it.
        hwnd_ = ::CreateWindow(
            _T("BUTTON"),
            text_.data(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            r_.x,
            r_.y,
            r_.x2 - r_.x1 + 1,
            r_.y2 - r_.y1 + 1,
            HWND_MESSAGE, // Start as message only window.
            NULL,
            app::id(),
            this);

        if (!hwnd_)
            throw_ex(nice_exception, "Unable to create button.");
    }

    void button::destroy() noexcept {
        ::DestroyWindow(id());
    }

    void text_edit::create() {

        // Create it.
        hwnd_ = ::CreateWindow(
            _T("EDIT"),
            text_.data(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            r_.x,
            r_.y,
            r_.x2 - r_.x1 + 1,
            r_.y2 - r_.y1 + 1,
            HWND_MESSAGE, // Start as message only window.
            NULL,
            app::id(),
            this);

        if (!hwnd_)
            throw_ex(nice_exception, "Unable to create text edit control.");
    }

    void text_edit::destroy() noexcept {
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