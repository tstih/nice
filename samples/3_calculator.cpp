#include <vector>

#include "nice.hpp"

using namespace nice;

class calc_wnd : public app_wnd {
public:
    calc_wnd() : app_wnd("Calculator", { 600,600 })
    {
        // Init layout manager.
        set_layout(hlayout()
            << (vlayout() << &b7_ << &b8_ << &b9_ << &bclr_)
            << (vlayout() << &b4_ << &b5_ << &b6_ << &beq_)
            << (vlayout() << &b1_ << &b2_ << &b3_ << &bdiv_)
            << (vlayout() << &bplus_ << &b0_ << &bminus_ << &bmul_)
        );
    }
private:
    // Calculator buttons (in order)
    button
        b7_{ "7" }, b8_{ "8" }, b9_{ "9" }, bclr_{ "C" },
        b4_{ "4" }, b5_{ "5" }, b6_{ "6" }, beq_{ "=" },
        b1_{ "1" }, b2_{ "2" }, b3_{ "3" }, bdiv_{ "/" },
        bplus_{ "+" }, b0_{ "0" }, bminus_{ "-" }, bmul_{ "*" };
    // Display digits.
    std::string digits_;
};

void program()
{
    app::run(calc_wnd());
}