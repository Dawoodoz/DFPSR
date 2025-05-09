
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

#include "DsrWindow.h"

#include "components/Panel.h"
#include "components/Button.h"
#include "components/ListBox.h"
#include "components/TextBox.h"
#include "components/Label.h"
#include "components/Picture.h"
#include "components/Toolbar.h"
#include "components/Menu.h"
// <<<< Include new components here

#include "../math/scalar.h"
#include "../../math/IVector.h"
#include "../../api/imageAPI.h"
#include "../../api/filterAPI.h"

using namespace dsr;

static bool initialized = false;
void dsr::gui_initialize() {
	if (!initialized) {
		// Register built-in components by name
		REGISTER_PERSISTENT_CLASS(Panel)
		REGISTER_PERSISTENT_CLASS(Button)
		REGISTER_PERSISTENT_CLASS(ListBox)
		REGISTER_PERSISTENT_CLASS(TextBox)
		REGISTER_PERSISTENT_CLASS(Label)
		REGISTER_PERSISTENT_CLASS(Picture)
		REGISTER_PERSISTENT_CLASS(Toolbar)
		REGISTER_PERSISTENT_CLASS(Menu)
		// <<<< Register new components here

		initialized = true;
	}
}

DsrWindow::DsrWindow(Handle<BackendWindow> backend)
 : backend(backend), innerWidth(backend->getWidth()), innerHeight(backend->getHeight()) {
	// Initialize the GUI system if needed
	gui_initialize();
	// Listen to mouse and keyboard events from the backend window
	this->backend->mouseEvent() = [this](const MouseEvent& event) {
		this->sendMouseEvent(event);
	};
	this->backend->keyboardEvent() = [this](const KeyboardEvent& event) {
		this->sendKeyboardEvent(event);
	};
	this->backend->closeEvent() = [this]() {
		this->sendCloseEvent();
	};
	// Receiving notifications about resizing should be done in the main panel
	this->backend->resizeEvent() = [this](int width, int height) {
		BackendWindow *backend = this->backend.getUnsafe();
		ImageRgbaU8 canvas = backend->getCanvas();
		this->innerWidth = width;
		this->innerHeight = height;
		if (image_getWidth(canvas) != width || image_getHeight(canvas) != height) {
			// Resize the image that holds everything drawn on the window
			backend->resizeCanvas(width, height);
			// Remove the old depth buffer, so that it will resize when being requested again
			this->removeDepthBuffer();
		}
		this->applyLayout();
	};
	this->resetInterface();
}

static void setBackendWindowHandle(Handle<VisualComponent> component, Handle<BackendWindow> windowHandle) {
	component->window = windowHandle;
	for (int c = 0; c < component->children.length(); c++) {
		setBackendWindowHandle(component->children[c], windowHandle);
	}
}

DsrWindow::~DsrWindow() {
	// Disconnect the backend window from all components, so that handles to components without a DsrWindow will not prevent the BackendWindow from being freed.
	setBackendWindowHandle(this->mainPanel, Handle<BackendWindow>());
}

void DsrWindow::applyLayout() {
	this->mainPanel->applyLayout(IRect(0, 0, this->getCanvasWidth(), this->getCanvasHeight()));
}

Handle<VisualComponent> DsrWindow::findComponentByName(ReadableString name) const {
	if (string_match(this->mainPanel->getName(), name)) {
		return this->mainPanel;
	} else {
		return this->mainPanel->findChildByName(name);
	}
}

Handle<VisualComponent> DsrWindow::findComponentByNameAndIndex(ReadableString name, int index) const {
	if (string_match(this->mainPanel->getName(), name) && this->mainPanel->getIndex() == index) {
		return this->mainPanel;
	} else {
		return this->mainPanel->findChildByNameAndIndex(name, index);
	}
}

Handle<VisualComponent> DsrWindow::getRootComponent() const {
	return this->mainPanel;
}

void DsrWindow::resetInterface() {
	// Create an empty main panel
	this->mainPanel = handle_dynamicCast<VisualComponent>(createPersistentClass("Panel"));
	if (this->mainPanel.isNull()) {
		throwError(U"DsrWindow::resetInterface: The window's Panel could not be created!");
	}
	this->mainPanel->setName("mainPanel");
	// Inherit handle to backend window to access the clipboard.
	this->mainPanel->window = this->backend;
	this->applyLayout();
}

void DsrWindow::loadInterfaceFromString(String layout, const ReadableString &fromPath) {
	// Load a tree structure of visual components from text
	this->mainPanel = handle_dynamicCast<VisualComponent>(createPersistentClassFromText(layout, fromPath));
	// Re-assign the backend window handle
	setBackendWindowHandle(this->mainPanel, this->backend);
	if (this->mainPanel.isNull()) {
		throwError(U"DsrWindow::loadInterfaceFromString: The window's root component could not be created!\n\nLayout:\n", layout, "\n");
	}
	this->applyLayout();
}

String DsrWindow::saveInterfaceToString() {
	return this->mainPanel->toString();
}

bool DsrWindow::executeEvents() {
	return this->backend->executeEvents();
}

void DsrWindow::sendMouseEvent(const MouseEvent& event) {
	this->lastMousePosition = event.position;
	// Components will receive scaled mouse coordinates by being drawn to the low-resolution canvas
	MouseEvent scaledEvent = event / this->pixelScale;
	// Send the global event
	this->callback_windowMouseEvent(scaledEvent);
	// To pass the event to the main panel, the panel has to be visible with the cursor inside of it,
	//   unless it is a drag move or button release in which the event should never be blocked.
	if ((this->mainPanel->getVisible() && this->mainPanel->pointIsInside(scaledEvent.position)) || event.mouseEventType == MouseEventType::MouseUp || this->mainPanel->holdCount > 0) {
		// In case of the root panel not covering the entire window, adjust input coordinates to the panel's local system.
		scaledEvent.position -= this->mainPanel->location.upperLeft();
		// Send to the main panel and its components
		this->mainPanel->sendMouseEvent(scaledEvent);
	}
}

void DsrWindow::sendKeyboardEvent(const KeyboardEvent& event) {
	// Send the global event
	this->callback_windowKeyboardEvent(event);
	// Send to the main panel and its components
	this->mainPanel->sendKeyboardEvent(event);
}

void DsrWindow::sendCloseEvent() {
	this->callback_windowCloseEvent();
}

int DsrWindow::getInnerWidth() {
	return this->innerWidth;
}

int DsrWindow::getInnerHeight() {
	return this->innerHeight;
}

int DsrWindow::getCanvasWidth() {
	return max(1, this->innerWidth / this->pixelScale);
}

int DsrWindow::getCanvasHeight() {
	return max(1, this->innerHeight / this->pixelScale);
}

AlignedImageF32 DsrWindow::getDepthBuffer() {
	this->backend->getCanvas();
	int smallWidth = getCanvasWidth();
	int smallHeight = getCanvasHeight();
	if (!image_exists(this->depthBuffer)
	  || image_getWidth(this->depthBuffer) != smallWidth
	  || image_getHeight(this->depthBuffer) != smallHeight) {
		this->depthBuffer = image_create_F32(smallWidth, smallHeight);
	}
	return this->depthBuffer;
}

void DsrWindow::removeDepthBuffer() {
	this->depthBuffer = AlignedImageF32();
}

int DsrWindow::getPixelScale() const {
	return this->pixelScale;
}

void DsrWindow::setPixelScale(int scale) {
	if (this->pixelScale != scale) {
		this->pixelScale = scale;
		// Update layout
		this->applyLayout();
		// The mouse moves relative to the canvas when scale changes
		this->sendMouseEvent(MouseEvent(MouseEventType::MouseMove, MouseKeyEnum::NoKey, this->lastMousePosition));
	}
}

void DsrWindow::setFullScreen(bool enabled) {
	if (this->backend->isFullScreen() != enabled) {
		this->backend->setFullScreen(enabled);
		// TODO: The mouse moves relative to the canvas when the window moves, but the new mouse location was never given.
		// How can mouse-move events be made consistent in applications when toggling full-screen without resorting to hacks?
		// Return the moved pixel offset from backend's setFullScreen?
	}
}

bool DsrWindow::isFullScreen() {
	return this->backend->isFullScreen();
}

void DsrWindow::drawComponents() {
	auto canvas = this->getCanvas();
	this->mainPanel->draw(canvas, IVector2D(0, 0));
}

AlignedImageRgbaU8 DsrWindow::getCanvas() {
	auto fullResolutionCanvas = this->backend->getCanvas();
	if (this->pixelScale > 1) {
		// Get low resolution canvas in deterministic RGBA pack order
		int smallWidth = getCanvasWidth();
		int smallHeight = getCanvasHeight();
		if (!image_exists(this->lowResolutionCanvas)
		 || image_getWidth(this->lowResolutionCanvas) != smallWidth
 		 || image_getHeight(this->lowResolutionCanvas) != smallHeight) {
			this->lowResolutionCanvas = image_create_RgbaU8_native(smallWidth, smallHeight, image_getPackOrderIndex(fullResolutionCanvas));
		}
		return this->lowResolutionCanvas;
	} else {
		// Get full resolution canvas in arbitrary pack order
		return fullResolutionCanvas;
	}
}

void DsrWindow::showCanvas() {
	if (this->pixelScale > 1 && image_exists(this->lowResolutionCanvas)) {
		// Use an exact pixel size, by cutting into the last row and column when not even
		//   This makes it easy to convert mouse coordinates using multiplication and division with pixelScale
		auto target = this->backend->getCanvas();
		auto source = this->getCanvas();
		filter_blockMagnify(target, source, this->pixelScale, this->pixelScale);
	}
	this->backend->showCanvas();
}

String DsrWindow::getTitle() {
	return this->backend->getTitle();
}

void DsrWindow::setTitle(const String &newTitle) {
	return this->backend->setTitle(newTitle);
}

void DsrWindow::applyTheme(VisualTheme theme) {
	this->mainPanel->applyTheme(theme);
}

VisualTheme DsrWindow::getTheme() {
	return this->mainPanel->getTheme();
}

