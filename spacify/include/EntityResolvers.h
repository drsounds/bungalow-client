#ifndef ENTITY_RESOLVERS_H
#define ENTITY_RESOLVERS_H

class Database;

namespace space {
class SPUriStore;

/**
 * Registers resolvers that turn spacify: entity URIs into JSON from the
 * local library database:
 *
 *   spacify:artist:<isni>  {uri, type, isni, name}
 *   spacify:track:<id>     {uri, type, id, name, version, isrc, release, artists}
 *
 * @p database must outlive the store's use of the resolvers.
 */
void registerEntityResolvers(SPUriStore& store, Database& database);
}

#endif // ENTITY_RESOLVERS_H
