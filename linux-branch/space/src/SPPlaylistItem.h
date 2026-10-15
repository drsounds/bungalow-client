#pragma once

#include "SPResource.h"

namespace space {
class SPPlaylistResource;
class SPPlaylistItemResource : public SPResource {
public:
  string playlist_uri;
  string node_uri;
  string user_uri;
  string comments;
  int position;
  unsigned long added_at;
  unsigned long removed_at;
};
}
