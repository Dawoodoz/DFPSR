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

#ifndef DFPSR_GUI_VISUALCOMPONENT
#define DFPSR_GUI_VISUALCOMPONENT

#include "../persistent/includePersistent.h"
#include "BackendWindow.h" // TODO: Separate event types from the window module
#include "FlexRegion.h"
#include "InputEvent.h"
#include "VisualTheme.h"
#include "../api/imageAPI.h"
#include "../api/drawAPI.h"

namespace dsr {

class VisualComponent : public Persistent {
PERSISTENT_DECLARATION(VisualComponent)
protected:
	// Parent component
	VisualComponent *parent = nullptr;
	// Child components
	List<std::shared_ptr<VisualComponent>> children;
	// Remember the component used for a drag event
	//   Ensures that mouse down events are followed by mouse up events on the same component
	int holdCount = 0;
	std::shared_ptr<VisualComponent> dragComponent;
	// Remember the focused component for keyboard input
	std::shared_ptr<VisualComponent> focusComponent;
	// Saved properties
	FlexRegion region;
	PersistentString name;
	PersistentInteger index;
	PersistentBoolean visible = PersistentBoolean(true);
	void declareAttributes(StructureDefinition &target) const override {
		target.declareAttribute(U"Name");
		target.declareAttribute(U"Index");
		target.declareAttribute(U"Visible");
		target.declareAttribute(U"Left");
		target.declareAttribute(U"Top");
		target.declareAttribute(U"Right");
		target.declareAttribute(U"Bottom");
	}
public:
	Persistent* findAttribute(const ReadableString &name) override {
		if (string_caseInsensitiveMatch(name, U"Name")) {
			return &(this->name);
		} else if (string_caseInsensitiveMatch(name, U"Index")) {
			return &(this->index);
		} else if (string_caseInsensitiveMatch(name, U"Visible")) {
			return &(this->visible);
		} else if (string_caseInsensitiveMatch(name, U"Left")) {
			return &(this->region.sides[0]);
		} else if (string_caseInsensitiveMatch(name, U"Top")) {
			return &(this->region.sides[1]);
		} else if (string_caseInsensitiveMatch(name, U"Right")) {
			return &(this->region.sides[2]);
		} else if (string_caseInsensitiveMatch(name, U"Bottom")) {
			return &(this->region.sides[3]);
		} else {
			return nullptr;
		}
	}
protected:
	// Generated automatically from region in applyLayout
	IRect location;
	void setLocation(const IRect &newLocation);
	// Applied reqursively while selecting the correct theme
	VisualTheme theme;
public:
	void applyTheme(VisualTheme theme);
	VisualTheme getTheme() const;
public:
	// Constructor
	VisualComponent();
	// Destructor
	virtual ~VisualComponent();
public:
	virtual bool isContainer() const;
	IRect getLocation() const;
	IVector2D getSize() const;
	void setRegion(const FlexRegion &newRegion);
	FlexRegion getRegion() const;
	void setVisible(bool visible);
	bool getVisible() const;
	void setName(const String& newName);
	String getName() const;
	void setIndex(int index);
	int getIndex() const;
public:
	// Callbacks
	DECLARE_CALLBACK(pressedEvent, emptyCallback);
	DECLARE_CALLBACK(mouseDownEvent, mouseCallback);
	DECLARE_CALLBACK(mouseUpEvent, mouseCallback);
	DECLARE_CALLBACK(mouseMoveEvent, mouseCallback);
	DECLARE_CALLBACK(mouseScrollEvent, mouseCallback);
	DECLARE_CALLBACK(keyDownEvent, keyboardCallback);
	DECLARE_CALLBACK(keyUpEvent, keyboardCallback);
	DECLARE_CALLBACK(keyTypeEvent, keyboardCallback);
	DECLARE_CALLBACK(selectEvent, indexCallback);
private:
	std::shared_ptr<VisualComponent> getDirectChild(const IVector2D& pixelPosition, bool includeInvisible);
public:
	// Draw the component
	//   The component is responsible for drawing the component at this->location + offset.
	//   The caller is responsible for drawing the background for any pixels in the component that might not be fully opaque.
	//   If drawing out of bound, the pixels that are outside should be skipped without any warning nor crash.
	//   To clip the drawing of a component when calling this, give a sub-image and adjust for the new coordinate system using offset.
	//   If not implemented, a rectangle will mark the region where the component will be drawn as a reference.
	// targetImage is the image being drawn to.
	// offset is the upper left corner of the parent container relative to the image.
	//   Clipping will affect the offset by being relative to the new sub-image.
	void draw(ImageRgbaU8& targetImage, const IVector2D& offset);
	// A basic request to have the component itself drawn to targetImage at relativeLocation.
	//   The method is responsible for clipping without a warning when bound is outside of targetImage.
	//   Clipping will be common if the component is drawn using multiple dirty rectangles to save time.
	virtual void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation);
	// Draw the component while skipping pixels outside of clipRegion
	//   Multiple calls with non-overlapping clip regions should be equivalent to one call with the union of all clip regions.
	//     This means that the draw methods should handle border clipping so that no extra borderlines or rounded edges appear from nowhere.
	//     Example:
	//       drawClipped(i, o, IRect(0, 0, 20, 20)) // Full region
	//           <=>
	//       drawClipped(i, o, IRect(0, 0, 10, 20)) // Left half
	//       drawClipped(i, o, IRect(10, 0, 10, 20)) // Right half
	//   Drawing with the whole target image as a clip region should be equivalent to a corresponding call to draw with the same targetImage and offset.
	//     draw(i, o) <=> drawClipped(i, o, IRect(0, 0, i.width(), i.height()))
	void drawClipped(ImageRgbaU8 targetImage, const IVector2D& offset, const IRect& clipRegion);

// TODO: Distinguish from the generic version
	// Add a child component
	//   Preconditions:
	//     The parent's component type is a container.
	//     The child does not already have a parent.
	void addChildComponent(std::shared_ptr<VisualComponent> child);
	// Called with any persistent type when constructing child components from text
	bool addChild(std::shared_ptr<Persistent> child) override;
	// Called when saving to text
	int getChildCount() const override;
	std::shared_ptr<Persistent> getChild(int index) const override;

// TODO: Reuse in Persistent
	// Returns true iff child is a member of the component
	//   Searches recursively
	bool hasChild(VisualComponent *child) const;
	bool hasChild(std::shared_ptr<VisualComponent> child) const;

	// Find the first child component with the requested name using a case sensitive match.
	//   If mustExist is true, failure will raise an exception directly.
	//   Returns: A shared pointer to the child or null if not found.
	std::shared_ptr<VisualComponent> findChildByName(ReadableString name, bool mustExist) const;
	std::shared_ptr<VisualComponent> findChildByNameAndIndex(ReadableString name, int index, bool mustExist) const;
	// Detach the component from any parent
	void detachFromParent();

	// Adapt the location based on the region
	//   parentWidth must be the current width of the parent container
	//   parentHeight must be the current height of the parent container
	// Override to apply a custom behaviour, which may be useful for fixed size components.
	virtual void applyLayout(IVector2D parentSize);
	// Called after the component has been created, moved or resized.
	virtual void updateLocationEvent(const IRect& oldLocation, const IRect& newLocation);
	// Returns true iff the pixel with its upper left corner at pixelPosition is inside the component.
	// A rectangular bound check with location is used by default.
	// The caller is responsible for checking if the component is visible when needed.
	virtual bool pointIsInside(const IVector2D& pixelPosition);
	// Get a reference to the topmost child
	// Invisible components are ignored by default, but includeInvisible can be enabled to change that.
	// Returns an empty reference if the pixel position didn't hit anything inside.
	// Since the root component might not be heap allocated, it cannot return itself by reference.
	//   Use pointIsInside if your root component doesn't cover the whole window.
	std::shared_ptr<VisualComponent> getTopChild(const IVector2D& pixelPosition, bool includeInvisible = false);
	// Send a mouse down event to the component
	//   pixelPosition is relative to the parent container.
	//   The component is reponsible for bound checking, which can be used to either block the signal or pass to components below.
	void sendMouseEvent(const MouseEvent& event);
	void sendKeyboardEvent(const KeyboardEvent& event);
	// Defines what the component does when it has received an event that didn't hit any sub components on the way.
	//   pixelPosition is relative to the parent container.
	//   This is not a callback event.
	virtual void receiveMouseEvent(const MouseEvent& event);
	virtual void receiveKeyboardEvent(const KeyboardEvent& event);
	// Notifies when the theme has been changed, so that temporary data depending on the theme can be replaced
	virtual void changedTheme(VisualTheme newTheme);
	// Override to be notified about individual attribute changes
	virtual void changedAttribute(const ReadableString &name) {};
	// Override to be notified about location changes
	virtual void changedLocation(const IRect &oldLocation, const IRect &newLocation) {};
	// Custom call handler to manipulate components across a generic API
	virtual String call(const ReadableString &methodName, const ReadableString &arguments);
};

}

#endif

