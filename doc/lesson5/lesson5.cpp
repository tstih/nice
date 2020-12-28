#include <vector>

#include "nice.hpp"

using namespace ni;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Scrible", { 800,600 })
    {
        drawing_ = false;

        paint.connect(this, &main_wnd::on_paint);
        mouse_down.connect(this, &main_wnd::on_mouse_down);
        mouse_up.connect(this, &main_wnd::on_mouse_up);
        mouse_move.connect(this, &main_wnd::on_mouse_move);
    }
private:
    std::vector<std::vector<pt>> strokes_;
    bool drawing_;

    bool on_paint(const artist& a) {
        // No strokes to draw yet?
        if (strokes_.size() == 0) return true;
        for (auto s : strokes_) {
            auto iter = s.begin(); // First point.
            auto prev = *iter;
            for (advance(iter, 1); iter != s.end(); ++iter)
            {
                auto p = *iter;
                a.draw_line({ 0,0,0 }, prev, p);
                prev = p;
            }
        }
        return true;
    }

    bool on_mouse_down(const mouse_info& mi) {
        drawing_ = true;
        strokes_.push_back(std::vector<pt>());
        strokes_.back().push_back(mi.location);
        return true;
    }

    bool on_mouse_move(const mouse_info& mi) {
        if (drawing_) {
            strokes_.back().push_back(mi.location);
            repaint();
        }
        return true;
    }

    bool on_mouse_up(const mouse_info& mi) {
        drawing_ = false;
        strokes_.back().push_back(mi.location);
        repaint();
        return true;
    }
};

void program()
{
    app::run(main_wnd());
}