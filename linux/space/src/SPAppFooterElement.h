#pragma once
#include "SPHBoxElement.h"

namespace space {
class SPElement;
class SPButtonElement;
class SPTextElement;
class SPSeekBarElement;
class SPObject;
class SPEventArgs;

class SPAppFooterElement : public SPHBoxElement {
public:
	explicit SPAppFooterElement(SPElement *parent);
	SPAppFooterElement() = default;
	~SPAppFooterElement() override;

	string getType() override { return "appfooter"; }

	/// Called when BungalowApp dispatches a "playback:changed" event.
	int onPlaybackChanged(SPObject *sender, SPEventArgs *args);

	/// Called when the seek bar dispatches a "seek" event.
	int onSeek(SPObject *sender, SPEventArgs *args);

	/// Called when the play/pause button is clicked.
	int onPlayPause(SPObject *sender, SPEventArgs *args);

	/// Called when the previous button is clicked.
	int onPrev(SPObject *sender, SPEventArgs *args);

	/// Called when the next button is clicked.
	int onNext(SPObject *sender, SPEventArgs *args);
	void paint(int x, int y, SPGraphicsContext *c) override;

private:
	SPButtonElement *m_prevBtn = nullptr;
	SPButtonElement *m_playPauseBtn = nullptr;
	SPButtonElement *m_nextBtn = nullptr;
	SPTextElement *m_posLabel = nullptr;
	SPSeekBarElement *m_seekBar = nullptr;
	SPTextElement *m_durLabel = nullptr;

	/// Format milliseconds as "m:ss".
	static std::string formatTime(int ms);
};
} // namespace space
