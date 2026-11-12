#pragma once

#include <cstdint>
#include <string>

namespace space {

/**
 * Node — one row of a NodeDataSource: anything with an identity and a
 * display name. Rows are polymorphic objects that answer from the model they
 * wrap, so they are held by pointer and never copied.
 */
class Node {
public:
    virtual ~Node() = default;

    /** The node's identity within its data source. */
    virtual std::string id() const = 0;
    /** The name the node is shown and spoken of by. */
    virtual std::string name() const = 0;

    Node(const Node&)            = delete;
    Node& operator=(const Node&) = delete;

protected:
    Node() = default;
};

/** Entity — a node that can be referred to from elsewhere by a portable URI. */
class Entity : public Node {
public:
    /** The portable reference to this entity, e.g. to store in a playlist. */
    virtual std::string uri() const = 0;
};

/** MediaEntity — an entity that may have something to play. */
class MediaEntity : public Entity {
public:
    /** The direct playback URI, "" when there is nothing local to play. */
    virtual std::string playUri() const { return ""; }
    /** False greys the row out. Entities that open a page rather than play stay enabled. */
    virtual bool isPlayable() const { return !playUri().empty(); }
    /** The credited artists, for display in the play queue. */
    virtual std::string artistNames() const { return ""; }
    /** The library track that plays this entity, 0 when there is none. */
    virtual int64_t trackId() const { return 0; }
    /** The ISRC other services may resolve this entity by, "" when it has none. */
    virtual std::string isrc() const { return ""; }
};

} // namespace space
