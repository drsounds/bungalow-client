#include "SPSubscription.h"

space::SPSubscription::SPSubscription(Space *space, const string& uri) {
  this->space = space;
  this->uri = uri;
}
