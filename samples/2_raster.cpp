#include <cstdint>

#include "nice.hpp"

using namespace nice;

extern uint8_t tut_raster[];

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Raster", { 800,600 })
    {
        paint.connect(this, &main_wnd::on_paint);
    }
private:
    raster tut_{256,192,tut_raster};

    bool on_paint(const artist& a) {
        a.fill_rect({ 0,0,255 }, paint_area);
        a.draw_raster(tut_, { 0,0 });
        return true;
    }
};

void program()
{
    app::run(main_wnd());
}