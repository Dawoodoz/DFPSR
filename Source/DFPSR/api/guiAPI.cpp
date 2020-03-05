
// zlib open source license
//
// Copyright (c) 2019 David Forsgren Piuva
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

#define DFPSR_INTERNAL_ACCESS

#include "guiAPI.h"
#include "timeAPI.h"
#include "../gui/DsrWindow.h"

using namespace dsr;

// To be implemented outside of the core framework
std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height);

#define MUST_EXIST(OBJECT, METHOD) if (OBJECT.get() == nullptr) { throwError("The " #OBJECT " handle was null in " #METHOD "\n"); }

Window dsr::window_create(const String& title, int32_t width, int32_t height) {
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	std::shared_ptr<dsr::BackendWindow> backend = createBackendWindow(title, width, height);
	if (backend.get() != nullptr) {
		return std::make_shared<DsrWindow>(backend);
	} else {
		return std::shared_ptr<DsrWindow>();
	}
}

Window dsr::window_create_fullscreen(const String& title) {
	return std::make_shared<DsrWindow>(createBackendWindow(title, 0, 0));
}

bool dsr::window_exists(const Window& window) {
	return window.get() != nullptr;
}

bool dsr::component_exists(const Component& component) {
	return component.get() != nullptr;
}

void dsr::window_loadInterfaceFromString(const Window& window, const String& content) {
	MUST_EXIST(window, window_loadInterfaceFromString);
	window->loadInterfaceFromString(content);
}

void dsr::window_loadInterfaceFromFile(const Window& window, const ReadableString& filename) {
	MUST_EXIST(window, window_loadInterfaceFromFile);
	window->loadInterfaceFromString(string_load(filename));
}

String dsr::window_saveInterfaceToString(const Window& window) {
	MUST_EXIST(window, window_saveInterfaceToString);
	return window->saveInterfaceToString();
}

Component dsr::window_getRoot(const Window& window) {
	MUST_EXIST(window, window_getRoot);
	return window->getRootComponent();
}

Component dsr::window_findComponentByName(const Window& window, const ReadableString& name, bool mustExist) {
	MUST_EXIST(window, window_findComponentByName);
	return window->findComponentByName(name);
}

Component dsr::window_findComponentByNameAndIndex(const Window& window, const ReadableString& name, int index, bool mustExist) {
	MUST_EXIST(window, window_findComponentByNameAndIndex);
	return window->findComponentByNameAndIndex(name, index);
}

bool dsr::window_executeEvents(const Window& window) {
	MUST_EXIST(window, window_executeEvents);
	return window->executeEvents();
}
void dsr::window_drawComponents(const Window& window) {
	MUST_EXIST(window, window_drawComponents);
	window->drawComponents();
}
void dsr::window_showCanvas(const Window& window) {
	MUST_EXIST(window, window_showCanvas);
	window->showCanvas();
}

int dsr::window_getPixelScale(const Window& window) {
	MUST_EXIST(window, window_getPixelScale);
	return window->getPixelScale();
}
void dsr::window_setPixelScale(const Window& window, int scale) {
	MUST_EXIST(window, window_setPixelScale);
	window->setPixelScale(scale);
}

void dsr::window_setFullScreen(const Window& window, bool enabled) {
	MUST_EXIST(window, window_setFullScreen);
	window->setFullScreen(enabled);
}
bool dsr::window_isFullScreen(const Window& window) {
	MUST_EXIST(window, window_isFullScreen);
	return window->isFullScreen();
}

AlignedImageRgbaU8 dsr::window_getCanvas(const Window& window) {
	MUST_EXIST(window, window_getCanvas);
	return window->getCanvas();
}
AlignedImageF32 dsr::window_getDepthBuffer(const Window& window) {
	MUST_EXIST(window, window_getDepthBuffer);
	return window->getDepthBuffer();
}

int dsr::window_getCanvasWidth(const Window& window) {
	MUST_EXIST(window, window_getCanvasWidth);
	return window->getCanvasWidth();
}
int dsr::window_getCanvasHeight(const Window& window) {
	MUST_EXIST(window, window_getCanvasHeight);
	return window->getCanvasHeight();
}
int dsr::window_getInnerWidth(const Window& window) {
	MUST_EXIST(window, window_getInnerWidth);
	return window->getInnerWidth();
}
int dsr::window_getInnerHeight(const Window& window) {
	MUST_EXIST(window, window_getInnerHeight);
	return window->getInnerHeight();
}

void dsr::window_setMouseEvent(const Window& window, const MouseCallback& mouseEvent) {
	MUST_EXIST(window, window_setMouseEvent);
	window->windowMouseEvent() = mouseEvent;
}
void dsr::window_setKeyboardEvent(const Window& window, const KeyboardCallback& keyboardEvent) {
	MUST_EXIST(window, window_setKeyboardEvent);
	window->windowKeyboardEvent() = keyboardEvent;
}
void dsr::window_setCloseEvent(const Window& window, const EmptyCallback& closeEvent) {
	MUST_EXIST(window, window_setCloseEvent);
	window->windowCloseEvent() = closeEvent;
}

void dsr::component_setPressedEvent(const Component& component, const EmptyCallback& event) {
	MUST_EXIST(component, component_setPressedEvent);
	component->pressedEvent() = event;
}
void dsr::component_setMouseDownEvent(const Component& component, const MouseCallback& mouseEvent) {
	MUST_EXIST(component, component_setMouseDownEvent);
	component->mouseDownEvent() = mouseEvent;
}
void dsr::component_setMouseUpEvent(const Component& component, const MouseCallback& mouseEvent) {
	MUST_EXIST(component, component_setMouseUpEvent);
	component->mouseUpEvent() = mouseEvent;
}
void dsr::component_setMouseMoveEvent(const Component& component, const MouseCallback& mouseEvent) {
	MUST_EXIST(component, component_setMouseMoveEvent);
	component->mouseMoveEvent() = mouseEvent;
}
void dsr::component_setMouseScrollEvent(const Component& component, const MouseCallback& mouseEvent) {
	MUST_EXIST(component, component_setMouseScrollEvent);
	component->mouseScrollEvent() = mouseEvent;
}
void dsr::component_setKeyDownEvent(const Component& component, const KeyboardCallback& keyboardEvent) {
	MUST_EXIST(component, component_setKeyDownEvent);
	component->keyDownEvent() = keyboardEvent;
}
void dsr::component_setKeyUpEvent(const Component& component, const KeyboardCallback& keyboardEvent) {
	MUST_EXIST(component, component_setKeyUpEvent);
	component->keyUpEvent() = keyboardEvent;
}
void dsr::component_setKeyTypeEvent(const Component& component, const KeyboardCallback& keyboardEvent) {
	MUST_EXIST(component, component_setKeyTypeEvent);
	component->keyTypeEvent() = keyboardEvent;
}

bool dsr::component_hasProperty(const Component& component, const ReadableString& propertyName) {
	MUST_EXIST(component, component_hasProperty);
	Persistent* target = component->findAttribute(propertyName);
	return target != nullptr;
}

ReturnCode dsr::component_setProperty(const Component& component, const ReadableString& propertyName, const ReadableString& value, bool mustAssign) {
	MUST_EXIST(component, component_setProperty_string);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustAssign) {
			throwError("component_setProperty_string: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return ReturnCode::KeyNotFound;
	} else {
		if (target->assignValue(value)) {
			return ReturnCode::Good;
		} else {
			if (mustAssign) {
				throwError("component_setProperty_string: The input ", value, " could not be assigned to property ", propertyName, " because of incorrect format.\n");
			}
			return ReturnCode::ParsingFailure;
		}
	}
}
String dsr::component_getProperty(const Component& component, const ReadableString& propertyName, bool mustExist) {
	MUST_EXIST(component, component_getProperty_string);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustExist) {
			throwError("component_getProperty_string: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return U"";
	} else {
		return component->toString();
	}
}

void dsr::window_applyTheme(const Window& window, const VisualTheme& theme) {
	MUST_EXIST(window, window_applyTheme);
	MUST_EXIST(theme, window_applyTheme);
	window->applyTheme(theme);
}
