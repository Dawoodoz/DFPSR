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

// A reusable method for calling the media machine that allow providing additional variables as style flags.
MediaResult component_generateImage(VisualTheme theme, MediaMethod &method, int width, int height, int red, int green, int blue, int pressed = 0, int focused = 0, int hover = 0);

class VisualComponent;

// Bit flags for component states.
//  The size of ComponentState may change if running out of bits for new flags.
//  Each state should have a direct state and an indirect state, so that bitwise operations can be used to scan all states at once.
using ComponentState = uint32_t;
static const ComponentState componentState_focusDirect = 1 << 0; // Component being directly focused.
static const ComponentState componentState_focusIndirect = 1 << 1; // Contains the component being focused.
static const ComponentState componentState_hoverDirect = 1 << 2; // Component being hovered.
static const ComponentState componentState_hoverIndirect = 1 << 3; // Contains the component being hovered.
static const ComponentState componentState_showingOverlayDirect = 1 << 4; // The component will have drawOverlay called, if also visible.
static const ComponentState componentState_showingOverlayIndirect = 1 << 5; // The component contains a component drawing overlays.
static const ComponentState componentState_focus = componentState_focusDirect | componentState_focusIndirect; // Direct or indirect focus.
static const ComponentState componentState_hover = componentState_hoverDirect | componentState_hoverIndirect; // Direct or indirect hover.
static const ComponentState componentState_showingOverlay = componentState_showingOverlayDirect | componentState_showingOverlayIndirect; // Direct or indirect overlay.
static const ComponentState componentState_direct   = 0b01010101010101010101010101010101;
static const ComponentState componentState_indirect = 0b10101010101010101010101010101010;

class VisualComponent : public Persistent {
PERSISTENT_DECLARATION(VisualComponent)
public: // Relations
	// Parent component
	VisualComponent *parent = nullptr;
	IRect givenSpace; // Remembering the local region that was reserved inside of the parent component.
	bool regionAccessed = false; // If someone requested access to the region, remember to update layout in case of new settings.
	// Child components
	List<std::shared_ptr<VisualComponent>> children;
	// Remember the component used for a drag event.
	//   Ensures that mouse down events are followed by mouse up events on the same component.
	int holdCount = 0;
	// Marked for removal from the parent when set to true.
	bool detach = false;
	// Remember the pressed component for sending mouse move events outside of its region.
	std::shared_ptr<VisualComponent> dragComponent;
private: // States
	// Use methods to set the current state, then have it copied to previousState after calling updateStateEvent in sendNotifications.
	ComponentState currentState, previousState;
private: // State updates
	// TODO: Can a faster update be made using a limited traversal from changed component to root, while reusing the same code for the update?
	// Called after changing direct states.
	void updateIndirectStates();
	// Looking for recent state changes and sending notifications through updateStateEvent for each components that had a state change.
	//   Deferring update notifications using this makes sure that events that trigger updates get to finish before the next one starts.
	//   This reduces the risk of dead-locks, race-conditions, pointer errors...
	// Also checking which components are marked for removal and detaching them, so that the object is not deleted while a memeber method is being called.
	void sendNotifications();
	// Remove the zeroes in addMask from ones in the component and all child components.
	void applyStateAndMask(ComponentState keepMask);
	// Clears directStates from all other components sharing the root, iff unique is true.
	// Adds directStates to the component.
	// Updates indirect states based on direct states.
	void addStateBits(ComponentState directStates, bool unique);
	// Removes directStates to the component.
	// Updates indirect states based on direct states.
	void removeStateBits(ComponentState directStates);
public: // Focus is reset when a new component is focused.
	// It was clicked directly last time.
	inline bool isFocused() { return (this->currentState & componentState_focusDirect) != 0; }
	// One of its recursive children was clicked last time.
	inline bool ownsFocus() { return (this->currentState & (componentState_focusDirect | componentState_focusIndirect)) != 0; }
	// Remove focus from all of the component's children.
	void defocusChildren();
	// Give focus to a trail from root to the component.
	void makeFocused();
public: // Hover is reset before assigned again using a bit mask.
	// The cursor hovered within this component without being occluded.
	bool isHovered() { return (this->currentState & componentState_hoverDirect) != 0; }
	// The cursor hovered within its region, but one of its recursive children got the direct hover state.
	bool ownsHover() { return (this->currentState & (componentState_hoverDirect | componentState_hoverIndirect)) != 0; }
	// Make the component directly hovered and its parents indirectly hovered.
	void hover();
public: // Showing overlay
	inline bool showingOverlay() { return (this->currentState & componentState_showingOverlayDirect) != 0; }
	inline bool ownsOverlay() { return (this->currentState & componentState_showingOverlay) != 0; }
	void showOverlay();
	void hideOverlay();
public:
	// Saved properties
	FlexRegion region;
	PersistentString name;
	PersistentInteger index;
	PersistentBoolean visible = PersistentBoolean(true);
	void declareAttributes(StructureDefinition &target) const override;
public:
	Persistent* findAttribute(const ReadableString &name) override;
public:
	// Generated automatically from region in applyLayout
	IRect location;
	void setLocation(const IRect &newLocation);
	// Applied reqursively while selecting the correct theme
	VisualTheme theme = theme_getDefault();
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
	IRect getLocation();
	void setRegion(const FlexRegion &newRegion);
	FlexRegion getRegion() const;
	void setVisible(bool visible);
	bool getVisible() const;
	void setName(const String& newName);
	String getName() const;
	void setIndex(int index);
	int getIndex() const;
public: // Internal events that component classes override to know when something has changed.
	// Called after the component has been created, moved or resized.
	virtual void updateLocationEvent(const IRect& oldLocation, const IRect& newLocation);
	// Called after a component's state changed, when it is relatively safe to do so.
	//   All state changes will be sent at the same time, because state changes are often used to trigger other changes.
	//   Changes to the state made within the notification will not trigger new notifications, because the old state is saved after the call is finished.
	virtual void updateStateEvent(ComponentState oldState, ComponentState newState);
public: // Callbacks that the application use by assigning lambdas to specific components in the interface.
	DECLARE_CALLBACK(pressedEvent, emptyCallback);
	DECLARE_CALLBACK(destroyEvent, emptyCallback);
	DECLARE_CALLBACK(mouseDownEvent, mouseCallback);
	DECLARE_CALLBACK(mouseUpEvent, mouseCallback);
	DECLARE_CALLBACK(mouseMoveEvent, mouseCallback);
	DECLARE_CALLBACK(mouseScrollEvent, mouseCallback);
	DECLARE_CALLBACK(keyDownEvent, keyboardCallback);
	DECLARE_CALLBACK(keyUpEvent, keyboardCallback);
	DECLARE_CALLBACK(keyTypeEvent, keyboardCallback);
	DECLARE_CALLBACK(selectEvent, indexCallback);
public:
	// Returning a shader pointer to the topmost direct visible child that contains pixelPosition.
	//   The pixelPosition is relative to the called component's upper left corner.
	std::shared_ptr<VisualComponent> getDirectChild(const IVector2D& pixelPosition);
	// Returning a shared pointer to itself.
	//   Currently not working for the root component because of limitations in C++.
	std::shared_ptr<VisualComponent> getShared();
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
	// Draw the component itself to targetImage at relativeLocation.
	//   The method is responsible for clipping without a warning when bound is outside of targetImage.
	virtual void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation);
	// Draw the component's overlays on top of other components in the window.
	//   Overlays are drawn using absolute positions on the canvas.
	//   The absoluteOffset is the location of the component's upper left corner relative to the whole window's canvas.
	// Use for anything that needs to be drawn on top of other components without being clipped by any parent components.
	virtual void drawOverlay(ImageRgbaU8& targetImage, const IVector2D &absoluteOffset);
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

	// Returns true iff child is a member of the component
	//   Searches recursively
	bool hasChild(VisualComponent *child) const;
	bool hasChild(std::shared_ptr<VisualComponent> child) const;

	// Find the first child component with the requested name using a case sensitive match.
	//   Returns: A shared pointer to the child or null if not found.
	std::shared_ptr<VisualComponent> findChildByName(ReadableString name) const;
	std::shared_ptr<VisualComponent> findChildByNameAndIndex(ReadableString name, int index) const;
	// Detach the component from any parent
	void detachFromParent();

	// Adapt the location based on the space given by the parent.
	//   The given space is usually a rectangle starting at the origin with the same dimensions as the parent component.
	//   If the parent has decorations around the child components, the region may include some padding from which the flexible regions calculate the locations from in percents.
	//     For example: A given space from 10 to 90 pixels will have 0% at 10 and 100% at 90.
	//   A toolbar may give non-overlapping spaces that are assigned automatically to simplify the process of maintaining the layout while adding and removing child components.
	// TODO: How can internal changes to inner dimensions be detected by the parent to run this method again?
	virtual void applyLayout(const IRect& givenSpace);
	// Update layout when the component moved but the parent has the same dimensions
	void updateLayout();
	// TODO: Remake desiredDimensions into a private variable, so that one can update it when attributes are changed and notify parents that they need to change size and redraw images.
	// Parent components that place child components automatically can ask them what their minimum useful dimensions are in pixels, so that their text will be visible.
	// The component can still be resized to less than these dimensions, because the outer components can't give more space than what is given by the window.
	virtual IVector2D getDesiredDimensions();
	// Return true to turn off automatic drawing of and interaction with child components.
	virtual bool managesChildren();
	// Get the component's absolute position relative to the window's client region.
	//IVector2D getAbsolutePosition();
	// Calling updateLocationEvent without changing the location, to be used when a child component changed its desired dimensions from altering attributes.
	bool childChanged = false;
	// Called before rendering or getting mouse input in case that a child component changed desired dimensions.
	void updateChildLocations();
	// Returns true iff the pixel relative to the parent container's upper left corner is inside of the component.
	//   By default, it returns true when pixelPosition is within the component's location, because most component are solid.
	// The caller is responsible for checking if the component is visible (this->visible.value), so this method would return true if the pixelPosition is inside of an invisible component.
	virtual bool pointIsInside(const IVector2D& pixelPosition);
	// Returns true iff the pixelPosition relative to the parent container's upper left corner is inside of the component's overlay.
	// The caller is responsible for checking if the component is showing an overlay (this->showOverlay).
	virtual bool pointIsInsideOfOverlay(const IVector2D& pixelPosition);
	// Send a mouse down event to the component
	//   pixelPosition is relative to the component's own upper left corner. TODO: Does this make sense, or should it use parent coordinates?
	//   The component is reponsible for bound checking, which can be used to either block the signal or pass to components below.
	// If recursive is true, notifications will be supressed to prevent duplicate events when called from within receiveMouseEvent.
	void sendMouseEvent(const MouseEvent& event, bool recursive = false);
	void sendKeyboardEvent(const KeyboardEvent& event);
	// Defines what the component does when it has received an event that didn't hit any sub components on the way.
	//   pixelPosition is relative to the parent's (this->parent) upper left corner.
	//   This is not a callback event, but a way for the component to handle events.
	virtual void receiveMouseEvent(const MouseEvent& event);
	virtual void receiveKeyboardEvent(const KeyboardEvent& event);
	// Notifies when the theme has been changed, so that temporary data depending on the theme can be replaced.
	virtual void changedTheme(VisualTheme newTheme);
	// Override to be notified about individual attribute changes
	virtual void changedAttribute(const ReadableString &name);
	// Override to be notified about location changes
	virtual void changedLocation(const IRect &oldLocation, const IRect &newLocation) {};
	// Custom call handler to manipulate components across a generic API
	virtual String call(const ReadableString &methodName, const ReadableString &arguments);
};

}

#endif

