#ifndef SPACEBASE_H
#define SPACEBASE_H

namespace space {
    class Space;

    class SpaceBase {
    protected:
        Space *m_space = nullptr;

    public:
        void setSpace(Space *space) { m_space = space; }
        Space *getSpace() { return m_space; }
    };
}

#endif
