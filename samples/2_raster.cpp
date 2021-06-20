#include <cstdint>

#include "nice.hpp"

using namespace nice;

extern uint8_t tut_raster[];
extern uint8_t power_on_wav[];

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Raster", { 1023,512 })
    {
        paint.connect(this, &main_wnd::on_paint);
        audio_.play_wave_async(wav_);
    }
private:
    raster tut_{256,192,tut_raster};
    wave wav_ { power_on_wav };
    audio audio_;

    bool on_paint(const artist& a) {
        a.fill_rect({ 0,0,0xff }, paint_area);
        a.draw_raster(tut_, { 0,0 });
        return true;
    }
};

void program()
{
    app::run(main_wnd());
}