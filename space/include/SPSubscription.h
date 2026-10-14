#include "SPObservable.h"

namespace space {
class SPSubscription : public SPObservable {
public:
  string uri;
  Space *space;
  explicit SPSubscription(Space *space, const string& uri);
};
}