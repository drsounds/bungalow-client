#pragma once

#include <map>

#include "SPResource.h"
#include "SPObject.h"
#include "SPObservable.h"
#include "SPEventArgs.h"

namespace space {
  enum sp_store_event_type {
    add, update, remove
  };
  class SPStoreEventArgs : public SPEventArgs {
  public:
    sp_store_event_type type;
    SPResource *object;
    explicit SPStoreEventArgs(const sp_store_event_type type, SPResource *object);
  };
  class SPStore : public SPObject, SPObservable {
  public:
    std::map<string, SPResource *> values;
    SPResource *get(const string&);
    void set(const string&, SPResource *);
    bool contains(const string&);
    void remove(const string&);
  };
}
