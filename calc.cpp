#include "nice.hpp"

using namespace nice;

class main_wnd : public app_wnd {
public:
    main_wnd() : app_wnd("Calculator") {
        created.connect(this, &main_wnd::on_created);
    }
private:
    void on_created() {

    }
};

void program()
{
    app::run(::create<main_wnd>());
}