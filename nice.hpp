#ifndef _NICE_HPP
#define _NICE_HPP

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <iterator>

// Main entry point. You must provide this.
extern void program();

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
extern LRESULT CALLBACK WndProc(
	HWND hWnd, 
	UINT message, 
	WPARAM wParam, 
	LPARAM lParam);

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
	class rct {
	public:
		union { coord left; coord x; coord x1; };
		union { coord top; coord y;  coord y1; };
		union { coord right; coord x2; };
		union { coord bottom; coord y2; };
	};

	class pt {
	public:
		union { coord left; coord x; };
		union { coord top; coord y; };
	};

	class size {
	public:
		union { coord width; coord w; };
		union { coord height; coord h; };
	};

	class color {
	public:
		byte r;
		byte g;
		byte b;
		byte a;
	};

	// --- painter --------------------------------------------------
	class artist {
	public:
		void draw_rect(color c, const rct& r) {
			RECT rect = { r.left, r.top, r.right, r.bottom};
			HBRUSH brush = ::CreateSolidBrush(RGB(c.r, c.g, c.b));
			FrameRect(hdc_, &rect, brush);
			::DeleteObject(brush);
		}


	private:
		HDC hdc_; // Windows device context;

		friend class wnd;
		friend class app_wnd;
	};



	// --- window base ----------------------------------------------
	class wnd
	{
	public:
		wnd();
		virtual ~wnd();

		// Methods.
		virtual void conjure() = 0;

		// Properties.
		wnd_id id();
		void text(std::string t);
		std::string text();

		// Events.
		signal<> created;		// Window created.
		signal<> destroyed;		// Window destroyed.
		signal<std::shared_ptr<artist>> paint;
	protected:
		// Properties.
		wnd_id id_;
		std::string text_;

		// Map of all existing windows, used by WndProc to delegate messages.
		static std::map<HWND, wnd*> wnd_map; // TODO: smart pointer logic

		// Member callback.
		virtual result local_wnd_proc(msg_id id, par1 p1, par2 p2) = 0;

		friend LRESULT CALLBACK ::WndProc(
			HWND hWnd,
			UINT message,
			WPARAM wParam,
			LPARAM lParam);
	};

	wnd::wnd() : id_{ 0 } {}

	wnd::~wnd() {}

	wnd_id wnd::id() { return id_; }

	void wnd::text(std::string t)
	{
		text_ = t;
	}

	std::map<HWND, wnd*> wnd::wnd_map;

	// --- application window ---------------------------------------
	class app_wnd : public wnd
	{
	public:
		app_wnd(std::string text) : wnd() { text_ = text; class_ = "APP_WND"; }
		virtual ~app_wnd() {}
		void conjure();
	protected:
		result local_wnd_proc(msg_id id, par1 p1, par2 p2);
	private:
		std::string class_;
		WNDCLASSEX wcex_;
		// Window procedure is a friend function of wnd class.
		friend LRESULT CALLBACK ::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};

	result app_wnd::local_wnd_proc(msg_id id, par1 p1, par2 p2) {
		switch (id)
		{
		case WM_ERASEBKGND:
			break;
		case WM_PAINT:
			{
			PAINTSTRUCT ps;
			HDC hdc= BeginPaint(this->id(), &ps);
			auto art = std::make_shared<artist>();
			art->hdc_ = hdc;
			paint.emit(art);
			EndPaint(this->id(), &ps);
			}
			break;

		case WM_CREATE:
			created.emit();
			break;

		case WM_DESTROY:
			destroyed.emit();
			PostQuitMessage(0);
			break;

		default:
			return ::DefWindowProc(this->id(), id, p1, p2);
		}
		return 0;
	}


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

		// Create window.
		w->conjure();

		// Show and update main window.
		::ShowWindow(w->id(), SW_SHOWNORMAL);
		::UpdateWindow(w->id());

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
	void app_wnd::conjure() {

		// Register window.
		ZeroMemory(&wcex_, sizeof(WNDCLASSEX));
		wcex_.cbSize = sizeof(WNDCLASSEX);
		wcex_.lpfnWndProc = ::WndProc;
		wcex_.hInstance = app::id();
		wcex_.lpszClassName = class_.data();

		if (!::RegisterClassEx(&wcex_)) { } // TODO: exception.

		// Create it.
		id_ = ::CreateWindowEx(
			0,
			class_.data(),
			text_.data(),
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT, CW_USEDEFAULT,
			800, 600,
			NULL,
			NULL,
			app::id(),
			NULL);

		if (!id_) { } // TODO: exception.

		// Add to list of windows.
		wnd::wnd_map.insert(std::make_pair(id_, this));
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

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	auto it = nice::wnd::wnd_map.find(hWnd);
	if (it != nice::wnd::wnd_map.end())
		return it->second->local_wnd_proc(message, wParam, lParam);
	else
		return ::DefWindowProc(hWnd, message, wParam, lParam);
}

#elif __linux__ // #ifdef _WIN32


// --- includes -----------------------------------------------------
#include <gtk/gtk.h>


namespace nice {

	// --- types ----------------------------------------------------
	typedef GtkApplication* app_id;
	typedef GtkWidget* wnd_id;


	// --- window base ----------------------------------------------
	class wnd
	{
	public:
		wnd();
		virtual ~wnd();

		// Methods.
		virtual void conjure() = 0;

		// Properties.
		wnd_id id();
		void text(std::string t);
		std::string text();

	protected:
		// Properties.
		wnd_id id_;
		std::string text_;
	};

	wnd::wnd() : id_{ 0 } {}

	wnd::~wnd() {}

	wnd_id wnd::id() { return id_; }

	void wnd::text(std::string t)
	{
		text_ = t;
	}


	// --- application window ---------------------------------------
	class app_wnd : public wnd
	{
	public:
		app_wnd(std::string text) : wnd() { text_ = text; }
		virtual ~app_wnd() {}
	};


	// --- application ----------------------------------------------
	extern static void on_app_activate(GtkApplication *app, gpointer user_data)

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
		w->conjure();
	}


	// --- cross reference functions --------------------------------
	void app_wnd::conjure() {
		// Create window and set the id.
		id(gtk_application_window_new (app::id()));
  		gtk_window_set_title (GTK_WINDOW (id()), text());
  		gtk_window_set_default_size (GTK_WINDOW (id()), 200, 200);
  		gtk_widget_show (id());
	}
}

// --- startup code -------------------------------------------------
static void on_app_activate(GtkApplication *app, gpointer user_data) {
	program();
}

int main(int argc, char **argv) {
    
	// Set application id.
	nice::app::id(gtk_application_new("nice.app", G_APPLICATION_FLAGS_NONE));

	// Connect app. activation to on_app_activate.
	g_signal_connect(app::id(), "activate", G_CALLBACK(on_app_activate), NULL);
    auto status = g_application_run(G_APPLICATION(nice::app:id()), argc, argv);
    g_object_unref(app::id());

	// Finally, set the return code.
	app::ret_code_ = status;

	// And return it.
    return nice::app::ret_code();
}

#endif // #elif __linux__

#endif // #ifndef _NICE_HPP