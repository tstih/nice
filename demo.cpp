#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
	main_wnd() : app_wnd(L"Zora zori dan se bijeli.") {
		// Subscribe to event.
		paint.connect(this, &main_wnd::on_paint);
	}
private:
	void on_paint(std::shared_ptr<artist> a) const {
		a->draw_rect({ 255,0,0 }, { 10,10,200,100 });
	}
};

void program()
{
    nice::app::run(
		std::static_pointer_cast<nice::app_wnd>(
			std::make_shared<main_wnd>()
		)
	);
}