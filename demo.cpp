#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello world!")
    {
        // Subscribe to events.
        created.connect(this, &main_wnd::on_created);
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    button ok_{ "OK", { 100,150,196,176 } };
    text_edit name_{ { 50,120,200,146 } };
    font verdana10_ { "Verdana", 9_px };
    
    void on_created() {
        // Add both controls to window.
        layout_manager() << ok_ << name_;
    }

    // Paint handler, draws rectangle.
    void on_paint(const artist& a) {
        a.draw_rect({ 255,0,0 }, { 10,10,200,100 });
        a.draw_text(verdana10_, { 100,200 }, "Hello!");
    }
};

void program()
{
    app::run(main_wnd());
}