#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello paint!"), ok(nullptr) {
        
        // Subscribe to events.
        created.connect(this, &main_wnd::on_created);
        paint.connect(this, &main_wnd::on_paint);
    }
private:

    // OK button.
    std::shared_ptr<button> ok;
    void on_created() {
        // Create child controls.
        ok = ::create<button>("OK", rct{ 100,100,196,126 });
    }

    // Paint handler, draws rectangle.
    void on_paint(std::shared_ptr<artist> a) {
        a->draw_rect({ 255,0,0 }, { 10,10,200,100 });
    }
};

void program()
{
    app::run(::create<main_wnd>());
}