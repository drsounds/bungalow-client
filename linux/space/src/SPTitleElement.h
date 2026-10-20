#pragma once
#include "SPVBoxElement.h"
#include "SPTextElement.h"
#include <string>
using namespace std;
namespace space {
class SPTitleElement : public SPVBoxElement
{
    public:
        SPTitleElement();
        SPTitleElement(SPElement *parent);
        virtual ~SPTitleElement();
        string getTitle();
        void setTitle(string);
        string getEntity();
        void setEntity(string);
        virtual string getType() override {
            return "title";
        }
    protected:
        SPTextElement *title;
        SPTextElement *entity;
    private:
};
}
