#include "nice.hpp"

using namespace ni;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Hello world!", { 800,600 })
    {
        // Subscribe to the paint event.
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    // Paint handler, draws red, green and blue rectangles.
    bool on_paint(const artist& a) {
        a.draw_rect({ 255,0,0 }, { 100,100,600,400 });
        a.draw_rect({ 0,255,0 }, { 150,150,500,300 });
        a.draw_rect({ 0,0,255 }, { 200,200,400,200 });
        return true;
    }
};

void program()
{
    app::run(main_wnd());
}