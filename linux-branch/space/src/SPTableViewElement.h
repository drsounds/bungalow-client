#pragma once
#include "SPElement.h"
#include <string>
using namespace std;
namespace space {
class SPTableDataSource;
class SPTableElement : public SPElement
{
    public:
        SPTableDataSource *dataSource;
        SPTableElement();
        SPTableElement(SPElement *parent, SPTableDataSource *dataSource);
        virtual ~SPTableElement();
        virtual string getType();
        virtual void paint(int x, int y, SPGraphicsContext *g);
};
}
