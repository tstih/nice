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
    std::shared_ptr<text_edit> name;
    void on_created() {


        // Create child controls.
        name = ::create<text_edit>(rct{ 50,120,200,146 });
        ok = ::create<button>("OK", rct{ 100,150,196,176 });

        // And reparent.
        add(name);
        add(ok);
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