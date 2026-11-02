/**
 * @module auth
 *
 * sp://import/scripts/api/auth — third-party sign-in for apps.
 *
 * Mocked: every request succeeds after a short delay with a fake access
 * token, so apps can exercise their success paths. Set
 * localStorage["bungalow.mock.auth.fail"] = "1" to exercise failures.
 * The C++ bridge will replace this with a real OAuth dialog.
 *
 * Callbacks (all optional):
 *   onSuccess(accessToken, ttlSeconds)
 *   onFailure(error)
 *   onComplete()            after either of the above
 */

var MOCK_DELAY = 400;

function shouldFail() {
    try { return localStorage.getItem("bungalow.mock.auth.fail") === "1"; } catch (e) { return false; }
}

function fakeToken(provider) {
    var chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    var token = provider.toUpperCase().slice(0, 4) + "MOCK";
    for (var i = 0; i < 48; i++) token += chars.charAt(Math.floor(Math.random() * chars.length));
    return token;
}

function run(provider, details, callbacks) {
    callbacks = callbacks || {};
    console.log("[auth] mock " + provider + " sign-in", details);
    setTimeout(function () {
        try {
            if (shouldFail()) {
                if (callbacks.onFailure) callbacks.onFailure("mock_failure");
            } else if (callbacks.onSuccess) {
                callbacks.onSuccess(fakeToken(provider), 3600);
            }
        } finally {
            if (callbacks.onComplete) callbacks.onComplete();
        }
    }, MOCK_DELAY);
}

/** authenticateWithFacebook(appId, permissions, callbacks) */
exports.authenticateWithFacebook = function (appId, permissions, callbacks) {
    run("facebook", { appId: appId, permissions: permissions || [] }, callbacks);
};

/**
 * showAuthenticationDialog(url, redirectUri, callbacks) — generic OAuth:
 * onSuccess gets the redirect URL with a fake token/code appended.
 */
exports.showAuthenticationDialog = function (url, redirectUri, callbacks) {
    callbacks = callbacks || {};
    run("oauth", { url: url, redirectUri: redirectUri }, {
        onSuccess: function (token, ttl) {
            if (!callbacks.onSuccess) return;
            var sep = String(redirectUri || "").indexOf("#") === -1 ? "#" : "&";
            callbacks.onSuccess((redirectUri || "") + sep + "access_token=" + token + "&expires_in=" + ttl, ttl);
        },
        onFailure: callbacks.onFailure,
        onComplete: callbacks.onComplete
    });
};

exports.isMock = true;
