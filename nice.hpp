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
		void text(std::wstring t);
		std::wstring text();

		// Events.
		signal<> created;		// Window created.
		signal<> destroyed;		// Window destroyed.
	protected:
		// Properties.
		wnd_id id_;
		std::wstring text_;

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

	void wnd::text(std::wstring t)
	{
		text_ = t;
	}

	std::map<HWND, wnd*> wnd::wnd_map;

	// --- application window ---------------------------------------
	class app_wnd : public wnd
	{
	public:
		app_wnd(std::wstring text) : wnd() { text_ = text; class_ = L"APP_WND"; }
		virtual ~app_wnd() {}
		void conjure();
	protected:
		result local_wnd_proc(msg_id id, par1 p1, par2 p2);
	private:
		std::wstring class_;
		WNDCLASSEX wcex_;
		// Window procedure is a friend function of wnd class.
		friend LRESULT CALLBACK ::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	};

	result app_wnd::local_wnd_proc(msg_id id, par1 p1, par2 p2) {
		switch (id)
		{
		case WM_CREATE:
			created.emit();
			return 0;
		case WM_DESTROY:
			destroyed.emit();
			PostQuitMessage(0);
			return 0;
		default:
			return ::DefWindowProc(this->id(), id, p1, p2);
		}
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

// http://mech.math.msu.su/~nap/2/GWindow/xintro.html
// https://jan.newmarch.name/Wayland/ProgrammingClient/
// cc -o connect connect.c -lwayland-client

// --- includes -----------------------------------------------------
#include <wayland-client.h>

// --- startup code -------------------------------------------------
int main(int argc, char** argv) {

	// Connect.
	struct wl_display*  display = wl_display_connect(NULL);
	if (display == NULL) {} // TODO: exception.
	
	// Run main program.
	program();

	// Disconnect.
	wl_display_disconnect(display);

	// And return success;
	return nice::app::ret_code();
}

#endif // #elif __linux__

#endif // #ifndef _NICE_HPP