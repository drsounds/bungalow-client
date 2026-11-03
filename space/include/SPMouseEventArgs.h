#ifndef MOUSEEVENT_H
#define MOUSEEVENT_H
#include "SPEventArgs.h"
namespace space {
class SPMouseEventArgs : public SPEventArgs {
    int button;
    int x;
    int y;
    bool m_cmdDown = false;
    int m_wheelDeltaY = 0;
public:
    int getButton() { return this->button; }
    int getX()      { return this->x; }
    int getY()      { return this->y; }
    bool getCmdDown() const { return this->m_cmdDown; }
    /// For "mousewheel": pixels to scroll, positive towards the end of the content.
    int getWheelDeltaY() const { return this->m_wheelDeltaY; }
    void setWheelDeltaY(int value) { this->m_wheelDeltaY = value; }
    string getType() override { return "SPMouseEventArgs"; }
    SPMouseEventArgs(int btn, int x, int y, bool cmdDown = false)
        : button(btn), x(x), y(y), m_cmdDown(cmdDown) {}
};
}
#endif // MOUSEEVENT_H
