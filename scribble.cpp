#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Scribble") {
        paint.connect(this, &main_wnd::on_paint);
        mouse_move.connect(this, &main_wnd::on_mouse_move);
    }
private:
    void on_paint(std::shared_ptr<artist> a) {
    }

    void on_mouse_move(std::shared_ptr<mouse_info> mi) {

    }
};

void program()
{
    app::run(::create<main_wnd>());
}