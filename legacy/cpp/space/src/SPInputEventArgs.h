#pragma once
#include "SPEventArgs.h"
namespace space {
class SPInputEventArgs : public SPEventArgs
{
    public:
        SPInputEventArgs(string text);
        string getText() {
            return this->text;
        }
        virtual ~SPInputEventArgs();
    protected:
        string text;
    private:
};
}
