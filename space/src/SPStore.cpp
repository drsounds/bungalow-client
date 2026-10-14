#include "SPStore.h"

namespace space {
    SPStoreEventArgs::SPStoreEventArgs(const sp_store_event_type type, SPResource *object) : SPEventArgs() {
      this->type = type;
      this->object = object;
    }
    SPResource *SPStore::get(const string& key) {
      return this->values.at(key);
    }
    void SPStore::set(const string& key, SPResource *value) {
      this->values[key] = value;
      SPStoreEventArgs *args = new SPStoreEventArgs(sp_store_event_type::add, value);
      this->notify("update", this, args);
    }
    bool SPStore::contains(const string& key) {
      return this->values.count(key) > 0;
    }
    void SPStore::remove(const string&) {
      
    }
}