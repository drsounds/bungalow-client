#ifndef SPACEBASE_H
#define SPACEBASE_H

namespace space {
  class SpaceBase {
  protected:
    space::Space *m_space;
  public:
    void setSpace(space::Space *space) {
        this->m_space = space;
    }
    space::Space *getSpace() {
        return this->m_space;
    }
  };
}
#endif