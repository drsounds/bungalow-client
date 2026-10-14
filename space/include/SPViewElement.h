#ifndef VIEWELEMENT_H
#define VIEWELEMENT_H
#include "SPVBoxElement.h"
#include <string>
using namespace std;
namespace space {
    class SPViewElement : public SPVBoxElement
    {
    private:
		string m_hash;
    public:
        SPViewElement();
        SPViewElement(SPElement *parent);
        virtual ~SPViewElement();
        virtual void navigate(const string& uri);
        virtual void navigateBack(const string& uri) { navigate(uri); }
        virtual void navigateForward(const string& uri) { navigate(uri); }
        virtual bool acceptsUri(const string& uri);
    	string getType() override {
    		return "view";
    	}
    	string getHash() {
    		return m_hash;
    	}
    	virtual void setHash(const string& hash) {
    		this->m_hash = hash;
    	}
      virtual void scroll(int scrollX, int scrollY, int mouseX, int mouseY) override;
      virtual void pack() override;
	};
}
#endif // VIEWELEMENT_H
