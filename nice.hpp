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
    template <typename T>
    class resource {
    public:
        
        // Create and destroy patter for double phase construction.
        virtual T create() = 0;
        virtual void destroy() noexcept = 0; // Can happen in an exception!
        
        // Id setter.
        virtual void id(T id) { id_ = id; }

        // First access to id() creates the underlying native window,
        // unless eval_only is set.
        virtual T id(bool eval_only=false) const {
            // Cheat c++ big time, just this once. Fake lazy_create() const.
            if (!eval_only)
                const_cast<resource<T>*>(this)->lazy_create();
            // Return.
            return id_;
        }

    private:
        // Lazy evaluate resource.
        void lazy_create() {
            if (id_ == nullptr)
                id(create());
        }
        // Store resource value here.
        T id_{ nullptr };
    };



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



    // --- properties ------------------------------------------------------
    template<typename T>
    class property {
    public:
        property(
            std::function<void(T)> setter,
            std::function<T()> getter) :
            setter_(setter), getter_(getter) { }
        operator T() const { return getter_(); }
        property<T>& operator= (const T& value) { setter_(value); return *this; }
    private:
        std::function<void(T)> setter_;
        std::function<T()> getter_;
    };

    template<typename T>
    class value_property : public property<T> {
    public:
        value_property(
            std::function<void(T)> setter,
            std::function<T()> getter) :
            property(setter, getter) { }
        T& value() { return value_; }
    private:
        T value_;
    };

} // using nice


#ifdef _WIN32

// --- includes -----------------------------------------------------
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <strsafe.h>
#include <dwmapi.h>

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
    typedef HGDIOBJ art_id;


    // --- primitive structures -------------------------------------
    struct rct {
        union { coord left; coord x; coord x1; };
        union { coord top; coord y;  coord y1; };
        union { coord right; coord x2; };
        union { coord bottom; coord y2; };
        coord width() { return x2 - x1 + 1; }
        coord height() { return y2 - y1 + 1; }
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

    enum class font_weight {
        thin = 100,
        extralight = 200,
        light = 300,
        normal = 400,
        medium = 500,
        semibold = 600,
        bold = 700,
        extrabold = 800,
        heavy = 900
    };

    // --- non functional message structures ------------------------
    struct mouse_info {
    public:
        mouse_info(pt pt, bool lbutton = false, bool mbutton = false, bool rbutton = false, bool ctrl_down = false, bool shift_down = false) {
            location = pt;
            left_button = lbutton;
            middle_button = mbutton;
            right_button = rbutton;
            ctrl = ctrl_down;
            shift = shift_down;
        }
        pt location;
        bool left_button;
        bool middle_button;
        bool right_button;
        bool ctrl;
        bool shift;
    };

    // --- quantities -----------------------------------------------
    class percent
    {
        double percent_;

    public:
        class pc {};
        explicit constexpr percent(pc, double dpc) : percent_{ dpc } {}
    };

    constexpr percent operator "" _pc(long double dpc)
    {
        return percent{ percent::pc{}, static_cast<double>(dpc) };
    }

    class pixel
    {
        int pixel_;

    public:
        class px {};
        explicit constexpr pixel(px, int ipx) : pixel_{ ipx } {}
        int value() { return pixel_; }
    };

    constexpr pixel operator "" _px(unsigned long long ipx)
    {
        return pixel{ pixel::px{}, static_cast<int>(ipx) };
    }

    // --- font -----------------------------------------------------
    class font : public resource<HFONT> {
    public:
        font(std::string name, pixel px, font_weight weight = font_weight::normal
        ) {
            ::ZeroMemory(&lf_, sizeof(LOGFONT));
            lf_.lfHeight = px2screen(px);
            lf_.lfWeight = static_cast<LONG>(weight);
            ::StringCchCopy(lf_.lfFaceName, 32, name.data());
        }

        virtual ~font() { destroy(); }

        virtual HFONT create() {
            return ::CreateFontIndirect(&lf_);
        }

        virtual void destroy() noexcept {
            ::DeleteObject(id(true));
            id(nullptr);
        }

    protected:
        LOGFONT lf_;

        int px2screen(pixel px) {
            auto hdc = ::GetDC(NULL);
            int h = -::MulDiv(px.value(), GetDeviceCaps(hdc, LOGPIXELSY), 72);
            ::ReleaseDC(NULL, hdc);
            return h;
        }
    };

    // --- painter --------------------------------------------------
    class artist {
    public:
        artist(HDC hdc) {
            hdc_ = hdc;
        }

        // Method(s).
        void draw_rect(color c, rct r) const {
            RECT rect = { r.left, r.top, r.right, r.bottom };
            HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
            ::FrameRect(hdc_, &rect, brush);
            ::DeleteObject(brush);
        }

        void draw_text(const font& f, pt pt, std::string text) const {
            RECT r{ pt.x,pt.y,pt.x + 100,pt.y + 50 };
            auto prev_font = ::SelectObject(hdc_, f.id());
            ::DrawText(hdc_, text.data(), text.length(), &r, DT_SINGLELINE);
            ::SelectObject(hdc_, prev_font);
        }

    private:
        HDC hdc_; // Windows device context;
    };

    // --- layout manager -------------------------------------------
    class wnd;
    // Basic layout manager maximizes each child.
    struct pane {
    public:
        virtual void apply(rct r) {}; // Apply layouting.
    };
    struct composite_pane : public pane {
    public:
        composite_pane& composite_pane::operator<<(wnd& w);
        virtual composite_pane& operator<<(pane p)
        {
            panes_.push_back(p);
            return *this;
        }
        virtual void apply(rct r) { // Basic layout is max. each child
            for (pane& p : panes_)
                p.apply(r);
        } 
    protected:
        std::vector<pane> panes_;
    };
    struct wnd_pane : public composite_pane {
    public:
        wnd_pane(wnd* host) : host_(host) {}
        virtual wnd_pane& wnd_pane::operator<<(wnd& w);
        void apply(rct r) override;
    private:
        wnd* host_;
    };

    // --- native window --------------------------------------------
    class native_wnd : public resource<HWND> {
    public:
        // Ctor.
        native_wnd() {}
        virtual ~native_wnd() { destroy(); }

        // Destroy should be the same for all native windows.
        virtual void destroy() noexcept { ::DestroyWindow(id(true)); id(nullptr); }
    };

    class wnd : public native_wnd
    {
    public:

        // Ctor.
        wnd(std::string text = nullptr) { 
            text_ = text; 
        }

        // Window text.
        virtual void text(std::string txt) {
            ::SetWindowText(id(), txt.data());
        }

        virtual std::string text() {
            TCHAR sz[1024];
            ::GetWindowText(id(), sz, 1024);
            return sz;
        }

        wnd_pane& layout_manager() { return layout_manager_; };

        // Events of basic window.
        signal<> created;
        signal<> destroyed;
        signal<const artist&> paint;
        signal<const mouse_info&> mouse_move;
        signal<const mouse_info&> mouse_down;
        signal<const mouse_info&> mouse_up;

    protected:

        wnd_pane layout_manager_{ this };

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
                artist a(hdc);
                paint.emit(a);
                EndPaint(this->id(), &ps);
            }
            break;

            case WM_MOUSEMOVE:
            {
                mouse_info mi({ GET_X_LPARAM(p2), GET_Y_LPARAM(p2) });
                mouse_move.emit(mi);
            }
            break;

            case WM_SIZING:
            case WM_SIZE:
            {
                RECT cr;
                ::GetClientRect(this->id(), &cr);
                // Apply default layout.
                layout_manager().apply({ cr.left, cr.top, cr.right, cr.bottom });
                // And fall through...
            }

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
        app_wnd(std::string text) : wnd(text) { 
            class_ = "APP_WND";
        }

        virtual wnd_id create();

    protected:
        virtual result local_wnd_proc(msg_id id, par1 p1, par2 p2) {
            // Forward the message down the chain.
            auto r = wnd::local_wnd_proc(id, p1, p2);
            // If message is quit then post quit message...
            if (id == WM_ACTIVATE) {
                // Extend the frame into the client area.
                MARGINS margins;

                margins.cxLeftWidth = 0;      // 8
                margins.cxRightWidth = 0;    // 8
                margins.cyBottomHeight = 0; // 20
                margins.cyTopHeight = 0;       // 27

                auto hr = DwmExtendFrameIntoClientArea(this->id(), &margins);

                if (!SUCCEEDED(hr))
                {
                    // Handle the error.
                }

                //fCallDWP = true;
                //lRet = 0;
            } else if (id == WM_DESTROY)
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
        button(std::string text, rct r) : wnd(text) { r_ = r; }

        virtual wnd_id create();

    protected:
        rct r_;
    };


    // --- text edit ------------------------------------------------
    class text_edit : public wnd {
    public:
        text_edit(rct r) : wnd(""), r_(r) {}

        virtual wnd_id create();

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
        static void run(const app_wnd& w);

    private:
        static app_id id_;
        static int ret_code_;
    };

    app_id app::id_{ 0 };
    int app::ret_code_{ 0 };

    app_id app::id() { return app::id_; }
    void app::id(app_id id) { app::id_ = id; }
    int app::ret_code() { return app::ret_code_; }

    void app::run(const app_wnd& w) {

        // Show and update main window.
        ::ShowWindow(w.id(), SW_SHOWNORMAL);

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
    composite_pane& composite_pane::operator<<(wnd& w)
    {
        panes_.push_back(w.layout_manager());
        return *this;
    }

    wnd_pane& wnd_pane::operator<<(wnd& w)
    {
        // Add a child, if not already added.
        if (::GetParent(w.id()) != host_->id())
            ::SetParent(w.id(), host_->id());
        panes_.push_back(w.layout_manager());
        return *this;
    }

    void wnd_pane::apply(rct r) {
        // Check if position is already set?
        RECT curr_pos;
        ::GetClientRect(host_->id(), &curr_pos);
        if (r.left!=curr_pos.left||r.right!=curr_pos.right||r.top!=curr_pos.top||r.bottom!=curr_pos.bottom)
            ::MoveWindow(host_->id(), r.x, r.y, r.width(), r.height(), FALSE);

        // And rewire to children...
        composite_pane::apply(r);
    }

    wnd_id app_wnd::create() {

        // Register window.
        ::ZeroMemory(&wcex_, sizeof(WNDCLASSEX));
        wcex_.cbSize = sizeof(WNDCLASSEX);
        wcex_.lpfnWndProc = wnd::global_wnd_proc;
        wcex_.hInstance = app::id();
        wcex_.lpszClassName = class_.data();
        wcex_.hCursor = ::LoadCursor(NULL, IDC_ARROW);

        if (!::RegisterClassEx(&wcex_)) throw_ex(nice_exception, "Unable to register application window.");

        // Create it.
        HWND hwnd = ::CreateWindowEx(
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

        if (!hwnd)
            throw_ex(nice_exception, "Unable to create application window.");

        return hwnd;
    }
    
    wnd_id button::create() {

        // Create it.
        HWND hwnd = ::CreateWindow(
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

        if (!hwnd)
            throw_ex(nice_exception, "Unable to create button.");

        return hwnd;
    }

    wnd_id text_edit::create() {

        // Create it.
        HWND hwnd = ::CreateWindow(
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

        if (!hwnd)
            throw_ex(nice_exception, "Unable to create text edit control.");

        return hwnd;
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