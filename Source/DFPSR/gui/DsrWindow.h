
// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_GUI_DSRWINDOW
#define DFPSR_GUI_DSRWINDOW

#include <memory>
#include "../gui/VisualComponent.h"
#include "../gui/BackendWindow.h"
#include "../api/stringAPI.h"
#include "../api/types.h"

// The DSR window is responsible for connecting visual interfaces with the backend window.
//   An optional depth buffer is allocated on demand when requested, and kept until the window resizes.

namespace dsr {

// Called to register the default components before trying to construct visual components from text or names.
void gui_initialize();

class DsrWindow {
private:
	// Window backend
	std::shared_ptr<BackendWindow> backend;
	// The root component
	std::shared_ptr<VisualComponent> mainPanel;
	AlignedImageF32 depthBuffer;
	// The inner window dimensions that are synchronized with the canvas.
	//   The backend on the contrary may have its size changed before the resize event has been fetched.
	//   Getting the asynchronous window dimensions directly wouldn't be synchronized with the canvas.
	int innerWidth, innerHeight;
	// The last mouse position is used to create new mouse-move events when pixelScale changes.
	IVector2D lastMousePosition;
public:
	// Constructor
	explicit DsrWindow(std::shared_ptr<BackendWindow> backend);
	// Destructor
	virtual ~DsrWindow();
public:
	// GUI layout
		void applyLayout();

		// Component getters
		std::shared_ptr<VisualComponent> findComponentByName(ReadableString name) const;
		template <typename T>
		std::shared_ptr<T> findComponentByName(ReadableString name) const {
			return std::dynamic_pointer_cast<T>(this->findComponentByName(name));
		}
		std::shared_ptr<VisualComponent> findComponentByNameAndIndex(ReadableString name, int index) const;
		template <typename T>
		std::shared_ptr<T> findComponentByNameAndIndex(ReadableString name, int index) const {
			return std::dynamic_pointer_cast<T>(this->findComponentByNameAndIndex(name, index));
		}

		// Get the root component that contains all other components in the window
		std::shared_ptr<VisualComponent> getRootComponent() const;
		void resetInterface();
		void loadInterfaceFromString(String layout);
		String saveInterfaceToString();

public:
	// Events
		// Call to listen for all events given to the window
		//   This will interact with components and call registered events
		//   Returns true if any event was processed
		bool executeEvents();

		// Callback for any mouse event given to the window, before components receive the event
		DECLARE_CALLBACK(windowMouseEvent, mouseCallback);
		// Send a mouse event directly to the visual components
		//   Can be called manually for automatic testing
		void sendMouseEvent(const MouseEvent& event);

		// Callback for any keyboard event given to the window, before components receive the event
		DECLARE_CALLBACK(windowKeyboardEvent, keyboardCallback);
		// Send a keyboard event directly to the visual components
		//   Can be called manually for automatic testing
		void sendKeyboardEvent(const KeyboardEvent& event);

		// Callback for when the user tries to close the window
		DECLARE_CALLBACK(windowCloseEvent, emptyCallback);
		// Send a close event directly
		//   Can be called manually for automatic testing
		void sendCloseEvent();

private:
	// Upscaling information
		int pixelScale = 1;
		AlignedImageRgbaU8 lowResolutionCanvas;
public:
	// Upscaling interface
		int getPixelScale() const;
		void setPixelScale(int scale);

public:
	// Graphics
		// Get the color buffer for drawing or 3D rendering
		//   The resulting color buffer may be outdated after resizing the window and calling executeEvents()
		AlignedImageRgbaU8 getCanvas();
		// Get the depth buffer for 3D rendering
		//   The resulting depth buffer may be outdated after resizing the window and calling executeEvents()
		AlignedImageF32 getDepthBuffer();
		// Detach the depth buffer so that it can be freed
		//   Called automatically when the canvas resizes
		void removeDepthBuffer();
		// Draw components directly to the canvas in full resolution
		void drawComponents();
		// Show the canvas when an image is ready
		void showCanvas();
		// Canvas width in the pre-upscale resolution
		int getCanvasWidth();
		// Canvas height in the pre-upscale resolution
		int getCanvasHeight();

public:
	// Full-screen
	void setFullScreen(bool enabled);
	bool isFullScreen();

public:
	// Theme
		void applyTheme(VisualTheme theme);
		VisualTheme getTheme();

public:
	// Access to backend window
		// Full width after upscaling
		int getInnerWidth();
		// Full height after upscaling
		int getInnerHeight();
		String getTitle();
		void setTitle(const String &newTitle);
};

}

#endif

