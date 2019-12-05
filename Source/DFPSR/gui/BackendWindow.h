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

#ifndef DFPSR_BACKEND_WINDOW
#define DFPSR_BACKEND_WINDOW

#include <stdint.h>
#include <memory>
#include "InputEvent.h"
#include "../image/ImageRgbaU8.h"
#include "../api/imageAPI.h"
#include "../base/text.h"
#include "../collection/List.h"

namespace dsr {

// The class to use when porting the window manager to another operating system.
//   A simple interface for the most basic operations that a window can do.
//     * Show an image over the whole window
//     * Take input events
//   Minimalism reduces the cost of porting core functionality to new operating systems.
//     All other features should be optional.
class BackendWindow {
protected:
	String title;
	// Events
	List<InputEvent*> eventQueue;
	void queueInputEvent(InputEvent* event) {
		this->eventQueue.push(event);
	}
private:
	int requestingResize = false;
	int requestedWidth = 0;
	int requestedHeight = 0;
protected:
	// Request to resize the window.
	//   When the implementation receives a resize, call receiveWindowResize with the new dimensions.
	//     If requestingResize is already true, it will just overwrite the old request.
	//   Next call to executeEvents will then use it to resize the canvas.
	void receivedWindowResize(int width, int height) {
		this->requestingResize = true;
		this->requestedWidth = width;
		this->requestedHeight = height;
	}
public:
	BackendWindow() {}
	virtual ~BackendWindow() {}
	virtual void setFullScreen(bool enabled) = 0;
	virtual bool isFullScreen() = 0;
	virtual int getWidth() const = 0;
	virtual int getHeight() const = 0;
protected:
	// Back-end interface
	// Responsible for adding events to eventQueue
	virtual void prefetchEvents() = 0;
public:
	// Canvas interface
	virtual AlignedImageRgbaU8 getCanvas() = 0;
	virtual void showCanvas() = 0;
	virtual void resizeCanvas(int width, int height) = 0;
	virtual String getTitle() { return this->title; }
	virtual void setTitle(const String &newTitle) = 0;
	// Each callback declaration has a public variable and a public getter and setter
	DECLARE_CALLBACK(closeEvent, emptyCallback);
	DECLARE_CALLBACK(resizeEvent, sizeCallback);
	DECLARE_CALLBACK(keyboardEvent, keyboardCallback);
	DECLARE_CALLBACK(mouseEvent, mouseCallback);
	// Call executeEvents to run all callbacks collected in eventQueue
	//   Returns true if any event was processed
	bool executeEvents();
};

}

#endif

