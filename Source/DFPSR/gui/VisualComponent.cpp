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

#include <stdint.h>
#include "VisualComponent.h"
#include "../image/internal/imageInternal.h"

using namespace dsr;

PERSISTENT_DEFINITION(VisualComponent)

VisualComponent::VisualComponent() {}

VisualComponent::~VisualComponent() {
	// Let the children know that the parent component no longer exists.
	for (int i = 0; i < this->getChildCount(); i++) {
		this->children[i]->parent = nullptr;
	}
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

IVector2D VisualComponent::getSize() {
	return this->getLocation().size();
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
	this->setLocation(this->region.getNewLocation(this->parentSize));
}

void VisualComponent::applyLayout(IVector2D parentSize) {
	this->parentSize = parentSize;
	this->updateLayout();
}

void VisualComponent::updateLocationEvent(const IRect& oldLocation, const IRect& newLocation) {
	// Place each child component
	for (int i = 0; i < this->getChildCount(); i++) {
		this->children[i]->applyLayout(newLocation.size());
	}
}

// Offset may become non-zero when the origin is outside of targetImage from being clipped outside of the parent region
void VisualComponent::draw(ImageRgbaU8& targetImage, const IVector2D& offset) {
	if (this->getVisible()) {
		IRect containerBound = this->getLocation() + offset;
		this->drawSelf(targetImage, containerBound);
		// Draw each child component
		for (int i = 0; i < this->getChildCount(); i++) {
			this->children[i]->drawClipped(targetImage, containerBound.upperLeft(), containerBound);
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
		child->applyLayout(this->getSize());
		// Connect to the new parent
		this->children.push(child);
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
		// If the removed component is focused from the parent, then remove focus so that the parent is focused instead.
		if (parent->focusComponent.get() == this) {
			parent->focusComponent = std::shared_ptr<VisualComponent>();
		}
		// Iterate over all children in the parent component
		for (int i = 0; i < parent->getChildCount(); i++) {
			std::shared_ptr<VisualComponent> current = parent->children[i];
			if (current.get() == this) {
				current->parent = nullptr; // Assign null
				parent->children.remove(i);
				return;
			}
		}
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

// Non-recursive top-down search
std::shared_ptr<VisualComponent> VisualComponent::getDirectChild(const IVector2D& pixelPosition, bool includeInvisible) {
	// Iterate child components in reverse drawing order
	for (int i = this->getChildCount() - 1; i >= 0; i--) {
		std::shared_ptr<VisualComponent> currentChild = this->children[i];
		// Check if the point is inside the child component
		if ((currentChild->getVisible() || includeInvisible) && currentChild->pointIsInside(pixelPosition)) {
			return currentChild;
		}
	}
	// Return nothing if the point missed all child components
	return std::shared_ptr<VisualComponent>();
}

// Recursive top-down search
std::shared_ptr<VisualComponent> VisualComponent::getTopChild(const IVector2D& pixelPosition, bool includeInvisible) {
	// Iterate child components in reverse drawing order
	for (int i = this->getChildCount() - 1; i >= 0; i--) {
		std::shared_ptr<VisualComponent> currentChild = this->children[i];
		// Check if the point is inside the child component
		if ((currentChild->getVisible() || includeInvisible) && currentChild->pointIsInside(pixelPosition)) {
			// Check if a component inside the child component is even higher up
			std::shared_ptr<VisualComponent> subChild = currentChild->getTopChild(pixelPosition - this->getLocation().upperLeft(), includeInvisible);
			if (subChild.get() != nullptr) {
				return subChild;
			} else {
				return currentChild;
			}
		}
	}
	// Return nothing if the point missed all child components
	return std::shared_ptr<VisualComponent>();
}

void VisualComponent::sendMouseEvent(const MouseEvent& event) {
	// Convert to local coordinates recursively
	MouseEvent localEvent = event - this->getLocation().upperLeft();
	std::shared_ptr<VisualComponent> childComponent;
	// Grab a component on mouse down
	if (event.mouseEventType == MouseEventType::MouseDown) {
		childComponent = this->dragComponent = this->focusComponent = this->getDirectChild(localEvent.position, false);
		this->holdCount++;
	}
	if (this->holdCount > 0) {
		// If we're grabbing a component, keep sending events to it
		childComponent = this->dragComponent;
	} else if (this->getVisible() && this->pointIsInside(event.position)) {
		// If we're not grabbing a component, see if we can send the action to another component
		childComponent = this->getDirectChild(localEvent.position, false);
	}
	// Send the signal to a child component or itself
	if (childComponent.get() != nullptr) {
		childComponent->sendMouseEvent(localEvent);
	} else {
		this->receiveMouseEvent(event);
	}
	// Release a component on mouse up
	if (event.mouseEventType == MouseEventType::MouseUp) {
		this->holdCount--;
		if (this->holdCount <= 0) {
			this->dragComponent = std::shared_ptr<VisualComponent>(); // Abort drag
			this->holdCount = 0;
		}
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
	if (this->parent != nullptr) {
		// For child component, go back to the root and then follow the focus pointers to find out which component is focused within the whole tree.
		//   One cannot just check if the parent points back directly, because old pointers may be left from a previous route.
		VisualComponent *root = this; while (root->parent != nullptr) { root = root->parent; }
		VisualComponent *leaf = root; while (leaf->focusComponent.get() != nullptr) { leaf = leaf->focusComponent.get(); }
		return leaf == this; // Focused if the root component points back to this component.
	} else {
		// Root component is focused if it does not redirect its focus to a child component.
		return this->focusComponent.get() == nullptr; // Focused if no child is focused.
	}
}

MediaResult VisualComponent::generateImage(MediaMethod &method, int width, int height, int red, int green, int blue, int pressed) {
	return method.callUsingKeywords([this, width, height, red, green, blue, pressed](MediaMachine &machine, int methodIndex, int inputIndex, const ReadableString &argumentName){
		if (string_caseInsensitiveMatch(argumentName, U"width")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, width);
		} else if (string_caseInsensitiveMatch(argumentName, U"height")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, height);
		} else if (string_caseInsensitiveMatch(argumentName, U"pressed")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, pressed);
		} else if (string_caseInsensitiveMatch(argumentName, U"red")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, red);
		} else if (string_caseInsensitiveMatch(argumentName, U"green")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, green);
		} else if (string_caseInsensitiveMatch(argumentName, U"blue")) {
			machine_setInputByIndex(machine, methodIndex, inputIndex, blue);
		} else if (theme_assignMediaMachineArguments(this->theme, machine, methodIndex, inputIndex, argumentName)) {
			// Assigned by theme_assignMediaMachineArguments.
		} else {
			// TODO: Ask the theme for the argument using a specified style class for variations between different types of buttons, checkboxes, panels, et cetera.
			//       Throw an exception if the theme did not provide an input argument to its own media function.
			throwError(U"Unhandled input ", argumentName, U" requested by ", machine_getMethodName(machine, methodIndex), U" in the visual theme!\n");
		}
	});
}
