/**
 * @module googletracker
 *
 * sp://import/scripts/googletracker — new GoogleTracker("UA-xxxx").track(page)
 *
 * Analytics are not sent from Bungalow apps; calls are logged instead.
 */
exports.GoogleTracker = function (account) {
    this.account = account;
    this.track = function (page) {
        console.log("[googletracker] " + account + " track:", page);
    };
    this.trackEvent = function (category, action, label, value) {
        console.log("[googletracker] " + account + " event:", category, action, label, value);
    };
};
