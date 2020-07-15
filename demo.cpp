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
    std::unique_ptr<button> ok;
    void on_created() {
        // Create child controls.
        ok = std::unique_ptr<button>(
            ::create<button>(id(), "OK", rct{ 100,100,196,126 })
            ->text("CLOSE")
            );
    }

    // Paint handler, draws rectangle.
    void on_paint(std::shared_ptr<artist> a) {
        a->draw_rect({ 255,0,0 }, { 10,10,200,100 });
    }
};

void program()
{
    app::run(
        std::unique_ptr<main_wnd>(
            ::create<main_wnd>()
        )
    );
}