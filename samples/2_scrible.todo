#include <vector>

#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Scrible", { 800,600 })
    {
        drawing_ = false;
        ink_ = color{ 0,0,0 };

        menu main_menu = menu()
            << ( menu("File")
                << menu_command("New", this, &main_wnd::on_file_new)
                )
            << ( menu("Edit")
                << menu_command("Red", this, &main_wnd::on_edit_red)
                << menu_command("Green", this, &main_wnd::on_edit_green)
                << menu_command("Blue", this, &main_wnd::on_edit_blue)
                << menu_separator()
                << menu_command("Black", this, &main_wnd::on_edit_black)
                );

        set_menu(main_menu);

        paint.connect(this, &main_wnd::on_paint);
        mouse_down.connect(this, &main_wnd::on_mouse_down);
        mouse_up.connect(this, &main_wnd::on_mouse_up);
        mouse_move.connect(this, &main_wnd::on_mouse_move);
    }
private:

    class stroke {
    public:
        pt p;
        color c;
    };

    std::vector<std::vector<stroke>> strokes_;
    bool drawing_;
    color ink_;

    void on_file_new() {
        strokes_.clear();
        repaint();
    }

    void on_edit_red() {
        ink_ = color{ 255,0,0 };
        repaint();
    }

    void on_edit_green() {
        ink_ = color{ 0,255,0 };
        repaint();
    }

    void on_edit_blue() {
        ink_ = color{ 0,0,255 };
        repaint();
    }

    void on_edit_black() {
        ink_ = color{ 0,0,0 };
        repaint();
    }

    bool on_paint(const artist& a) {
        // No strokes to draw yet?
        if (strokes_.size() == 0) {
            // Clean background.

            return true;
        };
        for (auto s : strokes_) {
            auto iter = s.begin(); // First point.
            auto prev = *iter;
            for (advance(iter, 1); iter != s.end(); ++iter)
            {
                auto p = *iter;
                a.draw_line(p.c, prev.p, p.p);
                prev = p;
            }
        }
        return true;
    }

    bool on_mouse_down(const mouse_info& mi) {
        drawing_ = true;
        strokes_.push_back(std::vector<stroke>());
        strokes_.back().push_back({ mi.location, ink_ });
        return true;
    }

    bool on_mouse_move(const mouse_info& mi) {
        if (drawing_) {
            strokes_.back().push_back({ mi.location, ink_ });
            repaint();
        }
        return true;
    }

    bool on_mouse_up(const mouse_info& mi) {
        drawing_ = false;
        strokes_.back().push_back({ mi.location, ink_ });
        repaint();
        return true;
    }
};

void program()
{
    app::run(main_wnd());
}