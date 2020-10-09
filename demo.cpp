#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello world!"), ok_(nullptr), name_(nullptr) {
        
        // Subscribe to events.
        created.connect(this, &main_wnd::on_created);
        paint.connect(this, &main_wnd::on_paint);
    }
private:

    // OK button.
    std::shared_ptr<button> ok_;
    std::shared_ptr<text_edit> name_;
    std::shared_ptr<font> verdana10_;

    void on_created() {


        // Create child controls.
        name_ = ::create<text_edit>(rct{ 50,120,200,146 });
        ok_ = ::create<button>("OK", rct{ 100,150,196,176 });

        // Create font.
        verdana10_ = ::create<font>("Verdana",15);

        // And reparent.
        add(name_);
        add(ok_);
    }

    // Paint handler, draws rectangle.
    void on_paint(std::shared_ptr<artist> a) {
        a->draw_rect({ 255,0,0 }, { 10,10,200,100 });
        a->draw_text(verdana10_, pt{ 100,200 }, "Hello!");
    }
};

void program()
{
    app::run(::create<main_wnd>());
}