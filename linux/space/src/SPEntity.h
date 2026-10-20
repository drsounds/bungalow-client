#pragma once
#include <vector>
#include <string>
using namespace std;
typedef int (*entify_promise_callback)(void *sender, void *data);
namespace Entify {
class SPEntity;
class Resolver;
class Promise;
/***
 * Entity
 * @class
 **/
class SPEntity
{
        vector<Resolver *> resolvers;
    public:
            /**
             * Creates a new instance of Entity
             * @constructor
             **/
            SPEntity();

            /**
             * @function
             **/
            void request(string method, string uri, void *data);

            /***
             * @function
             * @param {Entify::Resolver *} resolver the resolver to register
             * @return {void}
             **/
            void registerResolver(Resolver *resolver);
            virtual ~SPEntity();
    protected:
};

class Resolver {
public:
    Resolver();

    /**
     * Does a request
     * @function
     * @param {string} method The method
     * @param {string} uri The uri
     * @param {string} data The data
     **/
    virtual Promise request(string method, string uri, void *data);

    /***
     * Checks if the current entity URI is covered by this resolver
     * @virtual
     * @function
     * @return {bool}
     * @param {string} uri the uri
     **/
    virtual bool acceptsUri(string uri);
};

/***
 * The promise
 * @class
 **/
class Promise {
private:
    entify_promise_callback _done;
    entify_promise_callback _fail;
    entify_promise_callback _always;
public:
    Promise();
    ~Promise();
    void done(entify_promise_callback callback);
    void fail(entify_promise_callback callback);
    void always(entify_promise_callback callback);
    void setDone(void *data);
    void setFail(void *data);
};
}

