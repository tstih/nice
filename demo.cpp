#include "nice.hpp"

void program()
{
    nice::app::run(std::make_shared<nice::app_wnd>(L"Hello World!"));
}
