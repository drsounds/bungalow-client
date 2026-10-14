#ifndef MOUSEEVENT_H
#define MOUSEEVENT_H
#include "SPEventArgs.h"
namespace space {
class SPMouseEventArgs : public SPEventArgs {
    int button;
    int x;
    int y;
    bool m_cmdDown = false;
public:
    int getButton() { return this->button; }
    int getX()      { return this->x; }
    int getY()      { return this->y; }
    bool getCmdDown() const { return this->m_cmdDown; }
    string getType() override { return "SPMouseEventArgs"; }
    SPMouseEventArgs(int btn, int x, int y, bool cmdDown = false)
        : button(btn), x(x), y(y), m_cmdDown(cmdDown) {}
};
}
#endif // MOUSEEVENT_H
