#ifndef SP_APPLICATION_CONTEXT_H
#define SP_APPLICATION_CONTEXT_H

#include "SPObservable.h"

#include <string>

namespace space {
    class SPApplicationContext {
    public:
        virtual ~SPApplicationContext() = default;
        virtual SPObservable& getPlaybackEvents() = 0;
        virtual bool hasCurrentPlayable() const = 0;
        virtual std::string currentPlayableUri() const = 0;
        virtual const std::string& getTrackTitle() const = 0;
        virtual const std::string& getArtistName() const = 0;
        virtual void seekTo(int positionMs) = 0;
        virtual void togglePlayPause() = 0;
        virtual void playbackPrev() = 0;
        virtual void playbackNext() = 0;

        static SPApplicationContext* current() { return currentContext; }
        static void setCurrent(SPApplicationContext* context) {
            currentContext = context;
        }

    private:
        inline static SPApplicationContext* currentContext = nullptr;
    };
}

#endif
