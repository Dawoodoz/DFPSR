// zlib open source license
//
// Copyright (c) 2018 to 2023 David Forsgren Piuva
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

#include <stdint.h>
#include "VisualComponent.h"
#include "../image/internal/imageInternal.h"

using namespace dsr;

PERSISTENT_DEFINITION(VisualComponent)

VisualComponent::VisualComponent() {}

VisualComponent::~VisualComponent() {
	this->callback_destroyEvent();
	// Let the children know that the parent component no longer exists.
	for (int i = 0; i < this->getChildCount(); i++) {
		this->children[i]->parent = nullptr;
	}
}

IVector2D VisualComponent::getDesiredDimensions() {
	// Unless this virtual method is overridden, toolbars and such will try to give these dimensions to the component.
	return IVector2D(32, 32);
}

bool VisualComponent::isContainer() const {
	return true;
}

IRect VisualComponent::getLocation() {
	// If someone requested access to Left, Top, Right or Bottom, regionAccessed will be true
	if (this->regionAccessed) {
		// Now that a fixed location is requested, we need to recalculate the location from the flexible region based on parent dimensions
		this->updateLayout();
		this->regionAccessed = false;
	}
	return this->location;
}

void VisualComponent::setRegion(const FlexRegion &newRegion) {
	this->region = newRegion;
}

FlexRegion VisualComponent::getRegion() const {
	return this->region;
}

void VisualComponent::setVisible(bool visible) {
	this->visible.value = visible;
}

bool VisualComponent::getVisible() const {
	return this->visible.value;
}

void VisualComponent::setName(const String& newName) {
	this->name.value = newName;
}

String VisualComponent::getName() const {
	return this->name.value;
}

void VisualComponent::setIndex(int newIndex) {
	this->index.value = newIndex;
}

int VisualComponent::getIndex() const {
	return this->index.value;
}

void VisualComponent::setLocation(const IRect &newLocation) {
	IRect oldLocation = this->location;
	this->location = newLocation;
	if (oldLocation != newLocation) {
		this->updateLocationEvent(oldLocation, newLocation);
	}
	this->changedLocation(oldLocation, newLocation);
}

void VisualComponent::updateLayout() {
	this->setLocation(this->region.getNewLocation(this->givenSpace));
}

void VisualComponent::applyLayout(const IRect& givenSpace) {
	this->givenSpace = givenSpace;
	this->updateLayout();
}

void VisualComponent::updateLocationEvent(const IRect& oldLocation, const IRect& newLocation) {
	// Place each child component
	for (int i = 0; i < this->getChildCount(); i++) {
		this->children[i]->applyLayout(IRect(0, 0, newLocation.width(), newLocation.height()));
	}
}

// Check if any change requires the child layout to update.
//   Used to realign members of toolbars after a desired dimension changed.
void VisualComponent::updateChildLocations() {
	if (this->childChanged) {
		this->updateLocationEvent(this->location, this->location);
		this->childChanged = false;
	}
}

// Overlays are only cropped by the entire canvas, so the offset is the upper left corner of component relative to the upper left corner of the canvas.
static void drawOverlays(ImageRgbaU8& targetImage, VisualComponent &component, const IVector2D& offset) {
	if (component.getVisible()) {
		// Draw the component's own overlays at the bottom. 
		component.drawOverlay(targetImage, offset - component.location.upperLeft());
		// Draw overlays in each child component on top.
		for (int i = 0; i < component.getChildCount(); i++) {
			drawOverlays(targetImage, *(component.children[i]), offset + component.children[i]->location.upperLeft());
		}
	}
}

// Offset may become non-zero when the origin is outside of targetImage from being clipped outside of the parent region
void VisualComponent::draw(ImageRgbaU8& targetImage, const IVector2D& offset) {
	if (this->getVisible()) {
		this->updateChildLocations();
		IRect containerBound = this->getLocation() + offset;
		this->drawSelf(targetImage, containerBound);
		// Draw each child component
		if (!this->managesChildren()) {
			for (int i = 0; i < this->getChildCount(); i++) {
				this->children[i]->drawClipped(targetImage, containerBound.upperLeft(), containerBound);
			}
		}
		// When drawing the root, start recursive drawing of all overlays.
		if (this->parent == nullptr) {
			drawOverlays(targetImage, *this, this->location.upperLeft());
		}
	}
}

void VisualComponent::drawClipped(ImageRgbaU8 targetImage, const IVector2D& offset, const IRect& clipRegion) {
	IRect finalRegion = IRect::cut(clipRegion, IRect(0, 0, image_getWidth(targetImage), image_getHeight(targetImage)));
	if (finalRegion.hasArea()) {
		// TODO: Optimize allocation of sub-images
		ImageRgbaU8 target = image_getSubImage(targetImage, finalRegion);
		this->draw(target, offset - finalRegion.upperLeft());
	}
}

// A red rectangle is drawn as a placeholder if the class couldn't be found
// TODO: Should the type name be remembered in the base class for serializing missing components?
void VisualComponent::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	draw_rectangle(targetImage, relativeLocation, ColorRgbaI32(200, 50, 50, 255));
}

void VisualComponent::drawOverlay(ImageRgbaU8& targetImage, const IVector2D &absoluteOffset) {}

// Manual use with the correct type
void VisualComponent::addChildComponent(std::shared_ptr<VisualComponent> child) {
	if (!this->isContainer()) {
		throwError(U"Cannot attach a child to a non-container parent component!\n");
	} else if (child.get() == this) {
		throwError(U"Cannot attach a component to itself!\n");
	} else if (child->hasChild(this)) {
		throwError(U"Cannot attach to its own parent as a child component!\n");
	} else {
		// Remove from any previous parent
		child->detachFromParent();
		// Update layout based on the new parent size
		child->applyLayout(IRect(0, 0, this->location.width(), this->location.height()));
		// Connect to the new parent
		this->children.push(child);
		this->childChanged = true;
		child->parent = this;
	}
}

// Automatic insertion from loading
bool VisualComponent::addChild(std::shared_ptr<Persistent> child) {
	// Try to cast from base class Persistent to derived class VisualComponent
	std::shared_ptr<VisualComponent> visualComponent = std::dynamic_pointer_cast<VisualComponent>(child);
	if (visualComponent.get() == nullptr) {
		return false; // Wrong type!
	} else {
		this->addChildComponent(visualComponent);
		return true; // Success!
	}
}

int VisualComponent::getChildCount() const {
	return this->children.length();
}

std::shared_ptr<Persistent> VisualComponent::getChild(int index) const {
	if (index >= 0 && index < this->children.length()) {
		return this->children[index];
	} else {
		return std::shared_ptr<Persistent>(); // Null handle for out of bound.
	}
}

void VisualComponent::detachFromParent() {
	// Check if there's a parent component
	VisualComponent *parent = this->parent;
	if (parent != nullptr) {
		parent->childChanged = true;
		// If the removed component is focused from the parent, then remove focus so that the parent is focused instead.
		if (parent->focusComponent.get() == this) {
			parent->defocusChildren();
		}
		// Find the component to detach among the child components.
		for (int i = 0; i < parent->getChildCount(); i++) {
			std::shared_ptr<VisualComponent> current = parent->children[i];
			if (current.get() == this) {
				// Disconnect parent from child.
				current->parent = nullptr;
				// Disconnect child from parent.
				parent->children.remove(i);
				return;
			}
		}
		// Any ongoing drag action will allow the component to get the mouse up event to finish transactions safely before being deleted by reference counting.
		//   Otherwise it may break program logic or cause crashes.
	}
}

bool VisualComponent::hasChild(VisualComponent *child) const {
	for (int i = 0; i < this->getChildCount(); i++) {
		std::shared_ptr<VisualComponent> current = this->children[i];
		if (current.get() == child) {
			return true; // Found the component
		} else {
			if (current->hasChild(child)) {
				return true; // Found the component recursively
			}
		}
	}
	return false; // Could not find the component
}

bool VisualComponent::hasChild(std::shared_ptr<VisualComponent> child) const {
	return this->hasChild(child.get());
}

std::shared_ptr<VisualComponent> VisualComponent::findChildByName(ReadableString name) const {
	for (int i = 0; i < this->getChildCount(); i++) {
		std::shared_ptr<VisualComponent> current = this->children[i];
		if (string_match(current->getName(), name)) {
			return current; // Found the component
		} else {
			std::shared_ptr<VisualComponent> searchResult = current->findChildByName(name);
			if (searchResult.get() != nullptr) {
				return searchResult; // Found the component recursively
			}
		}
	}
	return std::shared_ptr<VisualComponent>(); // Could not find the component
}

std::shared_ptr<VisualComponent> VisualComponent::findChildByNameAndIndex(ReadableString name, int index) const {
	for (int i = 0; i < this->getChildCount(); i++) {
		std::shared_ptr<VisualComponent> current = this->children[i];
		if (string_match(current->getName(), name) && current->getIndex() == index) {
			return current; // Found the component
		} else {
			std::shared_ptr<VisualComponent> searchResult = current->findChildByNameAndIndex(name, index);
			if (searchResult.get() != nullptr) {
				return searchResult; // Found the component recursively
			}
		}
	}
	return std::shared_ptr<VisualComponent>(); // Could not find the component
}

bool VisualComponent::pointIsInside(const IVector2D& pixelPosition) {
	return pixelPosition.x > this->location.left() && pixelPosition.x < this->location.right()
	    && pixelPosition.y > this->location.top() && pixelPosition.y < this->location.bottom();
}

bool VisualComponent::pointIsInsideOfOverlay(const IVector2D& pixelPosition) {
	return false;
}

// Non-recursive top-down search
std::shared_ptr<VisualComponent> VisualComponent::getDirectChild(const IVector2D& pixelPosition) {
	// Iterate child components in reverse drawing order
	for (int i = this->getChildCount() - 1; i >= 0; i--) {
		std::shared_ptr<VisualComponent> currentChild = this->children[i];
		// Check if the point is inside the child component
		if (currentChild->getVisible() && currentChild->pointIsInside(pixelPosition)) {
			return currentChild;
		}
	}
	// Return nothing if the point missed all child components
	return std::shared_ptr<VisualComponent>();
}

// TODO: Store a pointer to the window in each visual component, so that one can get the shared pointer to the root and get access to clipboard functionality.
std::shared_ptr<VisualComponent> VisualComponent::getShared() {
	VisualComponent *parent = this->parent;
	if (parent == nullptr) {
		// Not working for the root component, because that would require access to the window.
		return std::shared_ptr<VisualComponent>();
	} else {
		for (int c = 0; c < parent->children.length(); c++) {
			if (parent->children[c].get() == this) {
				return parent->children[c];
			}
		}
		// Not found in its own parent if the component tree is broken.
		return std::shared_ptr<VisualComponent>();
	}
}

// Remove its pointer to its child and the whole trail of focus.
void VisualComponent::defocusChildren() {
	// Using raw pointers because the this pointer is not reference counted.
	//   Components should not call arbitrary events that might detach components during processing, until a better way to handle this has been implemented.
	VisualComponent* parent = this;
	while (true) {
		// Get the parent's focused direct child.
		VisualComponent* child = parent->focusComponent.get();
		if (child == nullptr) {
			return; // Reached the end.
		} else {
			parent->focusComponent = std::shared_ptr<VisualComponent>(); // The parent removes the focus pointer from the child.
			child->focused = false; // Remember that it is not focused, for quick access.
			parent = child; // Prepare for the next iteration.
		}
	}
}

// Pre-condition: component != nullptr
// Post-condition: Returns the root of component
VisualComponent *getRoot(VisualComponent *component) {
	assert(component != nullptr);
	while (component->parent != nullptr) {
		component = component->parent;
	}
	return component;
}

// Create a chain of pointers from the root to this component
//   Any focus pointers that are not along the chain will not count but work as a memory for when one of its parents get focus again.
void VisualComponent::makeFocused() {
	VisualComponent* current = this;
	// Remove any focus tail behind the new focus end.
	current->defocusChildren();
	while (current != nullptr) {
		VisualComponent* parent = current->parent;
		if (parent == nullptr) {
			return;
		} else {
			VisualComponent* oldFocus = parent->focusComponent.get();
			if (oldFocus == current) {
				// When reaching a parent that already points back at the component being focused, there is nothing more to do.
				return;
			} else {
				if (oldFocus != nullptr) {
					// When reaching a parent that deviated to the old focus branch, follow it and defocus the old components.
					parent->defocusChildren();
				}
				parent->focusComponent = current->getShared();
				current->focused = true;
				current = parent;
			}
		}
	}
}

void VisualComponent::sendNotifications() {
	if (this->focused && !this->previouslyFocused) {
		this->gotFocus();
	} else if (!this->focused && this->previouslyFocused) {
		this->lostFocus();
	}
	this->previouslyFocused = this->focused;
	for (int i = this->getChildCount() - 1; i >= 0; i--) {
		this->children[i]->sendNotifications();
	}
}

// Find the topmost overlay by searching backwards with the parent last and returning a pointer to the component.
// The point is relative to the upper left corner of component.
static VisualComponent *getTopmostOverlay(VisualComponent *component, const IVector2D &point) {
	// Go through child components in reverse draw order to stop when reaching the one that is visible.
	for (int i = component->getChildCount() - 1; i >= 0; i--) {
		VisualComponent *result = getTopmostOverlay(component->children[i].get(), point - component->children[i]->location.upperLeft());
		if (result != nullptr) return result;
	}
	// Check itself behind child overlays.
	if (component->showingOverlay && component->pointIsInsideOfOverlay(point + component->location.upperLeft())) {
		return component;
	} else {
		return nullptr;
	}
}

// Get the upper left corner of child relative to the upper left corner of parent.
//   If parent is null or not a parent of child, then child's offset is relative to the window's canvas.
static IVector2D getTotalOffset(const VisualComponent *child, const VisualComponent *parent = nullptr) {
	IVector2D result;
	while ((child != nullptr) && (child != parent)) {
		result += child->location.upperLeft();
		child = child->parent;
	}
	return result;
}

// Takes events with points relative to the upper left corner of the called component.
void VisualComponent::sendMouseEvent(const MouseEvent& event, bool recursive) {
	// Update the layout if needed.
	this->updateChildLocations();
	// Get the point of interaction within the component being sent to,
	//   so that it can be used to find direct child components expressed
	//   relative to their container's upper left corner.
	// If a button is pressed down, this method will try to grab a component to begin mouse interaction.
	//   Grabbing with the dragComponent pointer makes sure that move and up events can be given even if the cursor moves outside of the component.
	VisualComponent *childComponent = nullptr;
	// Find the component to interact with, from pressing down or hovering.
	if (event.mouseEventType == MouseEventType::MouseDown || this->dragComponent.get() == nullptr) {
		// Check the overlays first when getting mouse events to the root component.
		if (this->parent == nullptr) {
			childComponent = getTopmostOverlay(this, event.position);
		}
		// Check for direct child components for passing on the event recursively.
		//   The sendMouseEvent method can be called recursively from a member of an overlay, so we can't know
		//   which component is at the top without asking the components that manage interaction with their children.
		if (childComponent == nullptr && !this->managesChildren()) {
			std::shared_ptr<VisualComponent> nextContainer = this->getDirectChild(event.position);
			if (nextContainer.get() != nullptr) {
				childComponent = nextContainer.get();
			}
		}
	} else if (dragComponent.get() != nullptr) {
		// If we're grabbing a component, keep sending events to it.
		childComponent = this->dragComponent.get();
	}
	// Grab any detected component on mouse down events.
	if (event.mouseEventType == MouseEventType::MouseDown && childComponent != nullptr) {
		childComponent->makeFocused();
		this->dragComponent = childComponent->getShared();
		this->holdCount++;
	}
	// Send the signal to a child component or itself.
	if (childComponent != nullptr) {		
		// Recalculate local offset through one or more levels of ownership.
		IVector2D offset = getTotalOffset(childComponent, this);
		MouseEvent localEvent = event;
		localEvent.position = event.position - offset;
		childComponent->sendMouseEvent(localEvent);
	} else {
		// If there is no child component found, interact directly with the parent.
		MouseEvent parentEvent = event;
		parentEvent.position += this->location.upperLeft();
		this->receiveMouseEvent(parentEvent);
	}
	// Release a component on mouse up.
	if (event.mouseEventType == MouseEventType::MouseUp) {
		this->holdCount--;
		if (this->holdCount <= 0) {
			this->dragComponent = std::shared_ptr<VisualComponent>(); // Abort drag.
			// Reset when we had more up than down events, in case that the root panel was created with a button already pressed.
			this->holdCount = 0;
		}
	}
	// Once all focusing and defocusing with arbitrary callbacks is over, send the focus notifications to the components that actually changed focus.
	if (this->parent == nullptr && !recursive) {
		this->sendNotifications();
	}
}

void VisualComponent::receiveMouseEvent(const MouseEvent& event) {
	if (event.mouseEventType == MouseEventType::MouseDown) {
		this->callback_mouseDownEvent(event);
	} else if (event.mouseEventType == MouseEventType::MouseUp) {
		this->callback_mouseUpEvent(event);
	} else if (event.mouseEventType == MouseEventType::MouseMove) {
		this->callback_mouseMoveEvent(event);
	} else if (event.mouseEventType == MouseEventType::Scroll) {
		this->callback_mouseScrollEvent(event);
	}
}

void VisualComponent::sendKeyboardEvent(const KeyboardEvent& event) {
	// Send the signal to a focused component or itself
	if (this->focusComponent.get() != nullptr) {
		this->focusComponent->sendKeyboardEvent(event);
	} else {
		this->receiveKeyboardEvent(event);
	}
	// Send focus events in case that any component changed focus.
	if (this->parent == nullptr) {
		this->sendNotifications();
	}
}

void VisualComponent::receiveKeyboardEvent(const KeyboardEvent& event) {
	if (event.keyboardEventType == KeyboardEventType::KeyDown) {
		this->callback_keyDownEvent(event);
	} else if (event.keyboardEventType == KeyboardEventType::KeyUp) {
		this->callback_keyUpEvent(event);
	} else if (event.keyboardEventType == KeyboardEventType::KeyType) {
		this->callback_keyTypeEvent(event);
	}
}

void VisualComponent::applyTheme(VisualTheme theme) {
	this->theme = theme;
	this->changedTheme(theme);
	for (int i = 0; i < this->getChildCount(); i++) {
		this->children[i] -> applyTheme(theme);
	}
}

VisualTheme VisualComponent::getTheme() const {
	return this->theme;
}

void VisualComponent::changedTheme(VisualTheme newTheme) {}

String VisualComponent::call(const ReadableString &methodName, const ReadableString &arguments) {
	throwError("Unimplemented custom call received");
	return U"";
}

bool VisualComponent::isFocused() {
	return this->focused && this->focusComponent.get() == nullptr;
}

bool VisualComponent::ownsFocus() {
	return this->focused;
}

// Override these in components to handle changes of focus without having to remember the previous focus.
void VisualComponent::gotFocus() {}
void VisualComponent::lostFocus() {}

bool VisualComponent::managesChildren() {
	return false;
}

MediaResult dsr::component_generateImage(VisualTheme theme, MediaMethod &method, int width, int height, int red, int green, int blue, int pressed, int focused, int hover) {
	return method.callUsingKeywords([&theme, &method, width, height, red, green, blue, pressed, focused, hover](MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName){
		if (string_caseInsensitiveMatch(argumentName, U"width")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, width);
		} else if (string_caseInsensitiveMatch(argumentName, U"height")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, height);
		} else if (string_caseInsensitiveMatch(argumentName, U"pressed")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, pressed);
		} else if (string_caseInsensitiveMatch(argumentName, U"focused")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, focused);
		} else if (string_caseInsensitiveMatch(argumentName, U"hover")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, hover);
		} else if (string_caseInsensitiveMatch(argumentName, U"red")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, red);
		} else if (string_caseInsensitiveMatch(argumentName, U"green")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, green);
		} else if (string_caseInsensitiveMatch(argumentName, U"blue")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, blue);
		} else if (theme_assignMediaMachineArguments(theme, method.contextIndex, machine, methodIndex, inputIndex, argumentName)) {
			// Assigned by theme_assignMediaMachineArguments.
		} else {
			// TODO: Ask the theme for the argument using a specified style class for variations between different types of buttons, checkboxes, panels, et cetera.
			//       Throw an exception if the theme did not provide an input argument to its own media function.
			throwError(U"Unhandled setting \"", argumentName, U"\" requested by the media method \"", machine_getMethodName(machine, methodIndex), U"\" in the visual theme!\n");
		}
	});
}
