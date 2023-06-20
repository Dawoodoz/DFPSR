
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
#include "fileAPI.h"

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

void dsr::window_loadInterfaceFromString(const Window& window, const String& content, const ReadableString &fromPath) {
	MUST_EXIST(window, window_loadInterfaceFromString);
	window->loadInterfaceFromString(content, fromPath);
}

void dsr::window_loadInterfaceFromString(const Window& window, const String& content) {
	MUST_EXIST(window, window_loadInterfaceFromString);
	window->loadInterfaceFromString(content, file_getCurrentPath());
}

void dsr::window_loadInterfaceFromFile(const Window& window, const ReadableString& filename) {
	MUST_EXIST(window, window_loadInterfaceFromFile);
	window->loadInterfaceFromString(string_load(filename), file_getRelativeParentFolder(filename));
}

String dsr::window_saveInterfaceToString(const Window& window) {
	MUST_EXIST(window, window_saveInterfaceToString);
	return window->saveInterfaceToString();
}

Component dsr::window_getRoot(const Window& window) {
	MUST_EXIST(window, window_getRoot);
	return window->getRootComponent();
}

Component dsr::component_createWithInterfaceFromString(Component& parent, const String& content, const ReadableString &fromPath) {
	MUST_EXIST(parent, component_createWithInterfaceFromString);
	Component result = std::dynamic_pointer_cast<VisualComponent>(createPersistentClassFromText(content, fromPath));
	if (result.get() == nullptr) {
		throwError(U"component_createWithInterfaceFromString: The component could not be created!\n\nLayout:\n", content, "\n");
	}
	parent->addChildComponent(result);
	return result;
}

Component dsr::component_createWithInterfaceFromString(Component& parent, const String& content) {
	return component_createWithInterfaceFromString(parent, content, file_getCurrentPath());
}

Component dsr::component_createWithInterfaceFromFile(Component& parent, const String& filename) {
	return component_createWithInterfaceFromString(parent, string_load(filename), file_getRelativeParentFolder(filename));
}

Component dsr::component_findChildByName(const Component& parent, const ReadableString& name, bool mustExist) {
	MUST_EXIST(parent, component_findChildByName);
	return parent->findChildByName(name);
}

Component dsr::component_findChildByNameAndIndex(const Component& parent, const ReadableString& name, int index, bool mustExist) {
	MUST_EXIST(parent, component_findChildByNameAndIndex);
	return parent->findChildByNameAndIndex(name, index);
}

Component dsr::window_findComponentByName(const Window& window, const ReadableString& name, bool mustExist) {
	MUST_EXIST(window, window_findComponentByName);
	Component result = window->findComponentByName(name);
	if (mustExist && result.get() == nullptr) {
		throwError(U"window_findComponentByName: No child component named ", name, " found!");
	}
	return result;
}

Component dsr::window_findComponentByNameAndIndex(const Window& window, const ReadableString& name, int index, bool mustExist) {
	MUST_EXIST(window, window_findComponentByNameAndIndex);
	Component result = window->findComponentByNameAndIndex(name, index);
	if (mustExist && result.get() == nullptr) {
		throwError(U"window_findComponentByName: No child component named ", name, " with index ", index, " found!");
	}
	return result;
}

int dsr::component_getChildCount(const Component& parent) {
	if (parent.get()) {
		return parent->getChildCount();
	} else {
		return -1;
	}
}

Component dsr::component_getChild(const Component& parent, int childIndex) {
	if (parent.get()) {
		return std::dynamic_pointer_cast<VisualComponent>(parent->getChild(childIndex));
	} else {
		return std::shared_ptr<VisualComponent>(); // Null handle
	}
}

static void findAllComponentsByName(const Component& component, const ReadableString& name, std::function<void(Component, int)> callback) {
	if (component_exists(component)) {
		// Check if the current component matches
		if (string_match(component->getName(), name)) {
			callback(component, component->getIndex());
		}
		// Search among child components
		int childCount = component_getChildCount(component);
		for (int childIndex = childCount - 1; childIndex >= 0; childIndex--) {
			findAllComponentsByName(component_getChild(component, childIndex), name, callback);
		}
	}
}
void dsr::window_findAllComponentsByName(const Window& window, const ReadableString& name, std::function<void(Component, int)> callback) {
	MUST_EXIST(window, window_findAllComponentsByName);
	findAllComponentsByName(window->getRootComponent(), name, callback);
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

bool dsr::window_setCursorVisibility(const Window& window, bool visible) {
	MUST_EXIST(window, window_setCursorVisibility);
	return window->backend->setCursorVisibility(visible);
}

bool dsr::window_getCursorVisibility(const Window& window) {
	MUST_EXIST(window, window_getCursorVisibility);
	return window->backend->visibleCursor;
}

void dsr::window_setCursorPosition(const Window& window, int x, int y) {
	MUST_EXIST(window, window_setCursorPosition);
	window->backend->setCursorPosition(x, y);
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
void dsr::component_setDestroyEvent(const Component& component, const EmptyCallback& event) {
	MUST_EXIST(component, component_setDestroyEvent);
	component->destroyEvent() = event;
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
void dsr::component_setSelectEvent(const Component& component, const IndexCallback& selectEvent) {
	MUST_EXIST(component, component_setSelectEvent);
	component->selectEvent() = selectEvent;
}

bool dsr::component_hasProperty(const Component& component, const ReadableString& propertyName) {
	MUST_EXIST(component, component_hasProperty);
	Persistent* target = component->findAttribute(propertyName);
	return target != nullptr;
}

ReturnCode dsr::component_setProperty(const Component& component, const ReadableString& propertyName, const ReadableString& value, const ReadableString& fromPath, bool mustAssign) {
	MUST_EXIST(component, component_setProperty);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustAssign) {
			throwError("component_setProperty: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return ReturnCode::KeyNotFound;
	} else {
		if (target->assignValue(value, fromPath)) {
			component->changedAttribute(propertyName);
			return ReturnCode::Good;
		} else {
			if (mustAssign) {
				throwError("component_setProperty: The input ", value, " could not be assigned to property ", propertyName, " because of incorrect format.\n");
			}
			return ReturnCode::ParsingFailure;
		}
	}
}
ReturnCode dsr::component_setProperty(const Component& component, const ReadableString& propertyName, const ReadableString& value, bool mustAssign) {
	return component_setProperty(component, propertyName, value, file_getCurrentPath(), mustAssign);
}

String dsr::component_getProperty(const Component& component, const ReadableString& propertyName, bool mustExist) {
	MUST_EXIST(component, component_getProperty);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustExist) {
			throwError("component_getProperty: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return U"";
	} else {
		return target->toString();
	}
}
ReturnCode dsr::component_setProperty_string(const Component& component, const ReadableString& propertyName, const ReadableString& value, bool mustAssign) {
	MUST_EXIST(component, component_setProperty_string);
	Persistent* target = component->findAttribute(propertyName);
	PersistentString* stringTarget = dynamic_cast<PersistentString*>(target);
	if (target == nullptr) {
		if (mustAssign) {
			throwError("component_setProperty_string: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return ReturnCode::KeyNotFound;
	} else if (stringTarget == nullptr) {
		if (mustAssign) {
			throwError("component_setProperty_string: ", propertyName, " in ", component->getClassName(), " was a ", target->getClassName(), " instead of a string.\n");
		}
		return ReturnCode::KeyNotFound;
	} else {
		stringTarget->value = value;
		component->changedAttribute(propertyName);
		return ReturnCode::Good;
	}
}
String dsr::component_getProperty_string(const Component& component, const ReadableString& propertyName, bool mustExist) {
	MUST_EXIST(component, component_getProperty_string);
	Persistent* target = component->findAttribute(propertyName);
	PersistentString* stringTarget = dynamic_cast<PersistentString*>(target);
	if (target == nullptr) {
		if (mustExist) {
			throwError("component_getProperty_string: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return U"";
	} else if (stringTarget == nullptr) {
		if (mustExist) {
			throwError("component_getProperty_string: ", propertyName, " in ", component->getClassName(), " was a ", target->getClassName(), " instead of a string.\n");
		}
		return U"";
	} else {
		return stringTarget->value;
	}
}
ReturnCode dsr::component_setProperty_integer(const Component& component, const ReadableString& propertyName, int64_t value, bool mustAssign) {
	MUST_EXIST(component, component_setProperty_integer);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustAssign) {
			throwError("component_setProperty_integer: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return ReturnCode::KeyNotFound;
	} else {
		PersistentInteger* integerTarget = dynamic_cast<PersistentInteger*>(target);
		PersistentBoolean* booleanTarget = dynamic_cast<PersistentBoolean*>(target);
		if (integerTarget != nullptr) {
			integerTarget->value = value;
			component->changedAttribute(propertyName);
			return ReturnCode::Good;
		} else if (booleanTarget != nullptr) {
			booleanTarget->value = value ? 1 : 0;
			component->changedAttribute(propertyName);
			return ReturnCode::Good;
		} else {
			if (mustAssign) {
				throwError("component_setProperty_integer: ", propertyName, " in ", component->getClassName(), " was a ", target->getClassName(), " instead of an integer or boolean.\n");
			}
			return ReturnCode::KeyNotFound;
		}
	}
}
int64_t dsr::component_getProperty_integer(const Component& component, const ReadableString& propertyName, bool mustExist, int64_t defaultValue) {
	MUST_EXIST(component, component_getProperty_integer);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustExist) {
			throwError("component_getProperty_integer: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return defaultValue;
	} else {
		PersistentInteger* integerTarget = dynamic_cast<PersistentInteger*>(target);
		PersistentBoolean* booleanTarget = dynamic_cast<PersistentBoolean*>(target);
		if (integerTarget != nullptr) {
			return integerTarget->value;
		} else if (booleanTarget != nullptr) {
			return booleanTarget->value;
		} else {
			if (mustExist) {
				throwError("component_getProperty_integer: ", propertyName, " in ", component->getClassName(), " was a ", target->getClassName(), " instead of an integer or boolean.\n");
			}
			return defaultValue;
		}
	}
}

ReturnCode dsr::component_setProperty_image(const Component& component, const ReadableString& propertyName, const OrderedImageRgbaU8& value, bool mustAssign) {
	MUST_EXIST(component, component_setProperty_image);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustAssign) {
			throwError("component_setProperty_image: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return ReturnCode::KeyNotFound;
	} else {
		PersistentImage* imageTarget = dynamic_cast<PersistentImage*>(target);
		if (imageTarget != nullptr) {
			imageTarget->value = value;
			component->changedAttribute(propertyName);
			return ReturnCode::Good;
		} else {
			if (mustAssign) {
				throwError("component_setProperty_image: ", propertyName, " in ", component->getClassName(), " was a ", target->getClassName(), " instead of an image.\n");
			}
			return ReturnCode::KeyNotFound;
		}
	}
}
OrderedImageRgbaU8 dsr::component_getProperty_image(const Component& component, const ReadableString& propertyName, bool mustExist) {
	MUST_EXIST(component, component_getProperty_image);
	Persistent* target = component->findAttribute(propertyName);
	if (target == nullptr) {
		if (mustExist) {
			throwError("component_getProperty_image: ", propertyName, " in ", component->getClassName(), " could not be found.\n");
		}
		return OrderedImageRgbaU8();
	} else {
		PersistentImage* imageTarget = dynamic_cast<PersistentImage*>(target);
		if (imageTarget != nullptr) {
			return imageTarget->value;
		} else {
			if (mustExist) {
				throwError("component_getProperty_image: ", propertyName, " in ", component->getClassName(), " was a ", target->getClassName(), " instead of an image.\n");
			}
			return OrderedImageRgbaU8();
		}
	}
}

String dsr::component_call(const Component& component, const ReadableString& methodName, const ReadableString& arguments) {
	MUST_EXIST(component, component_call);
	return component->call(methodName, arguments);
}
String dsr::component_call(const Component& component, const ReadableString& methodName) {
	return component_call(component, methodName, U"");
}

void dsr::component_detachFromParent(const Component& component) {
	MUST_EXIST(component, component_detachFromParent);
	component->detach = true;
}

Component dsr::component_create(const Component& parent, const ReadableString& className, const ReadableString& identifierName, int index) {
	// Making sure that the default components exist before trying to create a component manually.
	gui_initialize();
	// Creating a component from the name
	Component child = std::dynamic_pointer_cast<VisualComponent>(createPersistentClass(className));
	if (child) {
		child->setName(identifierName);
		child->setIndex(index);
		// Attaching to a parent is optional, but convenient to do in the same call.
		if (parent) {
			parent->addChildComponent(child);
		}
	}
	return child;
}

void dsr::window_applyTheme(const Window& window, const VisualTheme& theme) {
	MUST_EXIST(window, window_applyTheme);
	MUST_EXIST(theme, window_applyTheme);
	window->applyTheme(theme);
}

void dsr::window_setTitle(Window& window, const dsr::String& title) {
	MUST_EXIST(window, window_setTitle);
	window->backend->setTitle(title);
}

String dsr::window_getTitle(Window& window) {
	MUST_EXIST(window, window_getTitle);
	return window->backend->getTitle();
}

String window_loadFromClipboard(Window& window, double timeoutInSeconds) {
	MUST_EXIST(window, window_loadFromClipboard);
	return window->backend->loadFromClipboard(timeoutInSeconds);
}

void window_saveToClipboard(Window& window, const ReadableString &text, double timeoutInSeconds) {
	MUST_EXIST(window, window_saveToClipboard);
	window->backend->saveToClipboard(text, timeoutInSeconds);
}
