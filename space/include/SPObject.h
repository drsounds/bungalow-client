#ifndef SPTYPE_H
#define SPTYPE_H
#include <string>
using namespace std;
namespace space {
    class Space;
    class SPObject {
    protected:
    	Space *m_space;
    public:
    	void setSpace(Space *space) {
    		this->m_space = space;
    	}
    	Space *getSpace() const {
    		return this->m_space;
    	}
        virtual ~SPObject() = default;

        virtual string getType() {
            return "Undefined";
        }
    };
}
#endif // SPTYPE_H
