// zlib open source license
//
// Copyright (c) 2017 to 2019 David Forsgren Piuva
// 
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
//    1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 
//    2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 
//    3. This notice may not be removed or altered from any source
//    distribution.

#include "BackendWindow.h"

using namespace dsr;

// Used when access to the external clipboard is not implemented.
static String backupClipboard;

ReadableString BackendWindow::loadFromClipboard(int64_t timeoutInMilliseconds) {
	sendWarning(U"loadFromClipboard is not implemented! Simulating clipboard using a variable within the same application.");
	return backupClipboard;
}

void BackendWindow::saveToClipboard(const ReadableString &text) {
	sendWarning(U"saveToClipboard is not implemented! Simulating clipboard using a variable within the same application.");
	backupClipboard = text;
}

bool BackendWindow::executeEvents() {
	bool executedEvent = false;
	this->prefetchEvents();
	// Execute any resize first
 	//   This makes sure that following events gets a canvas size synchronized with the window size
	if (this->requestingResize) {
		executedEvent = true;
		this->callback_resizeEvent(this->requestedWidth, this->requestedHeight);
		this->requestingResize = false;
	}
	// Look for events
	for (int e = 0; e < this->eventQueue.length(); e++) {
		InputEvent* event = this->eventQueue[e];
		if (event) {
			executedEvent = true;
			KeyboardEvent* kEvent = dynamic_cast<KeyboardEvent*>(event);
			MouseEvent* mEvent = dynamic_cast<MouseEvent*>(event);
			WindowEvent* wEvent = dynamic_cast<WindowEvent*>(event);
			if (kEvent) {
				this->callback_keyboardEvent(*kEvent);
			} else if (mEvent) {
				this->callback_mouseEvent(*mEvent);
			} else if (wEvent) {
				if (wEvent->windowEventType == WindowEventType::Close) {
					this->callback_closeEvent();
				} else if (wEvent->windowEventType == WindowEventType::Redraw) {
					//this->showCanvas();
				}
			}
		}
		delete event;
	}
	// Check for resize again in case that one was triggered by a callback
	if (this->requestingResize) {
		this->callback_resizeEvent(this->requestedWidth, this->requestedHeight);
		this->requestingResize = false;
	}
	// Clear the event queue to avoid repeating events
	this->eventQueue.clear();
	// Tell the caller if we did something
	return executedEvent;
}

