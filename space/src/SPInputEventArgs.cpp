#include "SPInputEventArgs.h"
namespace space {
SPInputEventArgs::SPInputEventArgs(string text)
    : SPEventArgs()
{
    //ctor
    this->text = text;
}

SPInputEventArgs::~SPInputEventArgs()
{
    //dtor
}
}
