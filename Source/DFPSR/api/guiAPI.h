
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

#ifndef DFPSR_API_GUI
#define DFPSR_API_GUI

#include "types.h"
#include "../api/stringAPI.h"
#include "../gui/InputEvent.h"

// createBackendWindow should be implemented outside of the core framework
//   Choose one of the window backends in SDK/native to compile and link with your application.
// std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height);

// Constness on handles doesn't propagate to any inner types
//   "const Comopnent&" only means that the writable Component handle can be created from a sub-expression
//   because the location where the handle is stored cannot be overwritten.
//   This allow getting a component by name and using it as an argument without being stored in a variable.

namespace dsr {

// Window Construction
	// A portable window will be wrapped around a native window backend supplied from a call to createBackendWindow.
	Window window_create(const dsr::String& title, int32_t width, int32_t height);
	// If the game starts in full screen, this constructor should be used instead.
	//   Otherwise the canvas may ask for the window's dimensions while the system still keeps the old dimensions due to delays.
	Window window_create_fullscreen(const dsr::String& title);
	// Returns true iff the window exists.
	bool window_exists(const Window& window);

// Set window title
	// Assigns the window's title.
	void window_setTitle(Window& window, const dsr::String& title);
	// Returns the window's title.
	String window_getTitle(Window& window);

// Clipboard
	// Most operating systems require that a window is associated with each request to access the clipboard.
	//   This allow anti-virus software to see the user consent for accessing the clipboard,
	//   because a key combination commonly associated with pasting text was pressed before the request was made.
	// Returns content from an internal or extenal clipboard.
	//   The internal clipboard within the application is used when the system specific backend has not implemented clipboard access.
	String window_loadFromClipboard(Window& window, double timeoutInSeconds);
	// Stores the text argument to an internal or extenal clipboard.
	//   The internal clipboard within the application is used when the system specific backend has not implemented clipboard access.
	void window_saveToClipboard(Window& window, const ReadableString &text, double timeoutInSeconds);

// Layout files
	// Loading an interface by parsing a layout file's content, with any external resources loaded relative to fromPath.
	//   Embedded images do not count as external resources, but file paths need fromPath in order to know from where they will be loaded.
	// Raises an exception if window doesn't exist.
	void window_loadInterfaceFromString(const Window& window, const dsr::String& content, const ReadableString &fromPath);
	// Loading an interface by parsing a layout file's content, with any external resources loaded relative to the current directory.
	//   Useful when your application has already assigned the current directory, or when all resources are embedded into the layout.
	// Raises an exception if window doesn't exist.
	void window_loadInterfaceFromString(const Window& window, const dsr::String& content);
	// Loading an interface by parsing a layout file loaded by filename.
	// Raises an exception if window doesn't exist.
	void window_loadInterfaceFromFile(const Window& window, const dsr::ReadableString& filename);
	// Store the interface back into a layout file.
	// Raises an exception if window doesn't exist.
	String window_saveInterfaceToString(const Window& window);

// Find a component
	// Get the component being stored directly in the window
	//   Raises an exception if window doesn't exist.
	//   There should always exist a root component where more components can be added recursively
	Component window_getRoot(const Window& window);
	// Returns a handle to the first matching component of the name in window.
	//                 For consistent behavior, make sure that only one component has the name, or use window_findComponentByNameAndIndex after giving each a unique index.
	// If mustExist is true, not finding any component with the name throws an exception.
	// Raises an exception if window doesn't exist.
	// Component names are case sensitive to reduce the risk of accidental naming conflicts among many components.
	Component window_findComponentByName(const Window& window, const ReadableString& name, bool mustExist = true);
	// Returns a handle to the first matching component of the name and index in window.
	// If mustExist is true, not finding any component with the name throws an exception.
	// Raises an exception if window doesn't exist.
	// Component names are case sensitive to reduce the risk of accidental naming conflicts among many components.
	Component window_findComponentByNameAndIndex(const Window& window, const ReadableString& name, int index, bool mustExist = true);
	// Calls back with the component handle and index for each match of the name.
	//   Can be used to count the number of components by incrementing a counter for each match.
	//   Can be used to detach all the matching components and have them automatically garbage collected by reference counting.
	//   The index can be used as a filter or to look up information.
	// To allow detaching components while iterating over the list of children, order is reversed for child components.
	// Raises an exception if window doesn't exist.
	// Component names are case sensitive to reduce the risk of accidental naming conflicts among many components.
	void window_findAllComponentsByName(const Window& window, const ReadableString& name, std::function<void(Component component, int index)> callback);

// The three main events to run in a loop at the end of the main function
	// If the window's event queue contained any resize of the window, the canvas and the depth buffer will be replaced during this call.
	//   New calls to window_getCanvas and window_getDepthBuffer are required after this call, because the window could be given a new size.
	//   Returns true iff any event were processed.
	//   By calling window_executeEvents in a loop while returning false, one can wait for input.
	//     Sleeping for 10 milliseconds is quite responsive while saving lots of battery.
	//     Only redrawing the regions that has changed (dirty rectangles et cetera) can further save power.
	//   Example:
	//     while (!window_executeEvents(window)) {
	//         time_sleepSeconds(0.01);
	//     }
	//     window_drawComponents(window);
	//     window_showCanvas(window);
	bool window_executeEvents(const Window& window);
	// Draw the root component and its children to the canvas.
	//   Raises an exception if window doesn't exist.
	void window_drawComponents(const Window& window);
	// Show the canvas.
	//   Raises an exception if window doesn't exist.
	void window_showCanvas(const Window& window);

// Pixel upscaling
//   The pixel-scale is the width and height of each canvas pixel when displayed on the window.
//     The color and depth buffers from window_getCanvas and window_getDepthBuffer will shrink to fit each pixel within the window.
//     Partial pixels at right and bottom sides are replaced with black padding,
//     so that mouse coordinates can be divided and multiplied evenly during scale conversion.
//   If using a higher value than the default 1, upscaling will be done during the call to window_showCanvas.
//     The backend window will receive the upscaled image to display over the whole window.
	// Gets the current pixel scale.
	//   Raises an exception if window doesn't exist.
	int window_getPixelScale(const Window& window);
	// Assigns a new pixel scale.
	//   Raises an exception if window doesn't exist.
	//   Just like when handling a window resize, this will replace the canvas and depth buffer.
	//     Any old handles to canvas and depth buffer will become useless, so fetch new image handles from the window to avoid black flickering.
	void window_setPixelScale(const Window& window, int scale);

// Cursor
	// Sets the cursor visibility for window, hiding if visible is false and showing if visible is true.
	// Returns true on success and false on failure.
	bool window_setCursorVisibility(const Window& window, bool visible);
	// Returns true iff the cursor is allowed to be displayed over window.
	bool window_getCursorVisibility(const Window& window);
	// Tries move the cursor so that its active point is at (x, y) within window.
	//   To get back the new cursor location and see if it worked, you must wait for the triggered mouse move event.
	//   It is impossible to know if the operation succeeded while returning, due to absolute position input devices overwriting the side-effect.
	// It is recommended to avoid this method if you can, but some games require capturing relative mouse movements.
	//   If you use this then make sure to test it carefully on all targeted operating systems and both relative and absolute input devices.
	// Pre-conditions:
	//   0 <= x < width
	//   0 <= y < height
	//   Moving the mouse outside of the window's region may cause undefined behavior between platforms.
	// WARNING!
	//   Only set the cursor location if you know for sure that the cursor is controlled using relative input devices, such as a mouse or track-pad.
	//   If an absolute input method is used, such as a stylus-pen, touch-screen or eye-tracker, the cursor will shake quickly.
	//   The solution is to let the user select between modes for relative and absolute inputs.
	//   You can for example let absolute input mode move the cursor freely within a dead zone and hover over edges
	//   to rotate the camera itself, when pattern detection noticed a stylus pen being used instead of a mouse.
	// WARNING!
	//   On MS-Windows the cursor will always move, but on X11 the cursor must be within the window's region.
	//   Only use this in full-screen mode to prevent getting stuck outside of the window.
	// WARNING!
	//   Due to potential race-conditions, you can not assume that the next mouse move event is generated by this call.
	void window_setCursorPosition(const Window& window, int x, int y);

// Full screen
	void window_setFullScreen(const Window& window, bool enabled);
	bool window_isFullScreen(const Window& window);

// Fetch the window's surfaces
//   Always get the canvas (and any depth buffer) after calls to window_executeEvents or window_setPixelScale,
//   because these may replace the canvas with a new size.
//   TODO: Prevent the backend window from freeing the memory while the canvas is still being used.
	// Get the canvas/color-buffer.
	//   Raises an exception if window doesn't exist.
	//   The canvas size will be smaller when pixelScale is larger, because the canvas has to fit inside the window.
	AlignedImageRgbaU8 window_getCanvas(const Window& window);
	// Get the depth buffer allocated on demand.
	//   Raises an exception if window doesn't exist.
	//   If you never call this method, no depth buffer will be allocated.
	//   If you call it at the same time as window_getCanvas, it will have the same size as the canvas.
	AlignedImageF32 window_getDepthBuffer(const Window& window);

// The low-resolution canvas and depth buffer dimensions are relative to mouse events given to components.
// Because component are drawn to the canvas and affected by upscaling.
	// Returns the width of the canvas.
	//   Raises an exception if window doesn't exist.
	int window_getCanvasWidth(const Window& window);
	// Returns the height of the canvas.
	//   Raises an exception if window doesn't exist.
	int window_getCanvasHeight(const Window& window);
// The window's inner dimensions are relative to mouse events received directly from the window at full pixel resolution.
	// Returns the inner width of the window.
	//   Raises an exception if window doesn't exist.
	int window_getInnerWidth(const Window& window);
	// Returns the inner height of the window.
	//   Raises an exception if window doesn't exist.
	int window_getInnerHeight(const Window& window);

// Direct window events
	// Listen to window mouse events.
	//   Raises an exception if window doesn't exist.
	//   event.mouseEventType gives the type of mouse event.
	//   event.key gives the key being used.
	void window_setMouseEvent(const Window& window, const MouseCallback& mouseEvent);
	// Listen to window keyboard events.
	//   Raises an exception if window doesn't exist.
	//   event.keyboardEventType gives the type of keyboard event.
	//   event.dsrKey gives the key being used.
	void window_setKeyboardEvent(const Window& window, const KeyboardCallback& keyboardEvent);
	// Listen to the window close event.
	//   Raises an exception if window doesn't exist.
	void window_setCloseEvent(const Window& window, const EmptyCallback& closeEvent);

// Components
	// Create a new component belonging to parent.
	// Returns a handle to the component or an empty handle if className have not been registered.
	//   Can use component_exists on the result to check if the operation was a success.
	// className must be the name of a registered persistent class inheriting from VisualComponent.
	// identifierName is used to find the component.
	// index can be used to identify multiple components with the same name without having to generate numbered names.
	Component component_create(const Component& parent, const ReadableString& className, const ReadableString& identifierName, int index = 0);
	// An alternative to window_loadInterfaceFromString when you want the interface loaded into a panel instead of the whole window.
	// Useful for dynamically creating and destroying interfaces to save memory and load the application faster.
	// Loading an interface by parsing a layout file's content, with any external resources loaded relative to fromPath.
	//   Embedded images do not count as external resources, but file paths need fromPath in order to know from where they will be loaded.
	// Raises an exception if parent doesn't exist.
	Component component_createWithInterfaceFromString(Component& parent, const String& content, const ReadableString &fromPath);
	// With fromPath implicitly being the current path.
	Component component_createWithInterfaceFromString(Component& parent, const String& content);
	// Loading the interface from a file and loading resources relative to filename's parent folder.
	Component component_createWithInterfaceFromFile(Component& parent, const String& filename);
	// Returns true iff the component exists.
	bool component_exists(const Component& component);
	// Removed the component from the parent.
	//   Does nothing if used against the root component.
	//   Make sure to erase any other references to the component if you want it erased, including parent pointers in any child components that may still be attached unless you detach them first.
	//   There is currently no attach function, because such a function would need runtime checks against cyclic dependencies from attaching a parent to its own child, and still be hard to debug once it happens.
	void component_detachFromParent(const Component& component);
	// Returns the number of direct (non-recursive) child components attached to parent, or -1 if parent is a null handle.
	int component_getChildCount(const Component& parent);
	// Returns the child at childIndex from parent, or an empty handle if parent does not exist or childIndex is out of bound.
	//   Child indices go from 0 to count - 1.
	//   The child index refers to the parent's list of children, not the child's index attribute.
	Component component_getChild(const Component& parent, int childIndex);
	// Returns a handle to the first matching component of the name in parent recursively.
	// Can be used instead of window_findComponentByName to further reduce the risk of name collisions, by only looking for recursive child components of a specific parent component.
	// If mustExist is true, not finding any component with the name within parent throws an exception.
	// Raises an exception if parent doesn't exist.
	// Component names are case sensitive to reduce the risk of accidental naming conflicts among many components.
	Component component_findChildByName(const Component& parent, const ReadableString& name, bool mustExist = true);
	// Returns a handle to the first matching component of the name and index in parent recursively.
	// If mustExist is true, not finding any component with the name within parent throws an exception.
	// Raises an exception if parent doesn't exist.
	// Component names are case sensitive to reduce the risk of accidental naming conflicts among many components.
	Component component_findChildByNameAndIndex(const Component& parent, const ReadableString& name, int index, bool mustExist = true);
	// Returns true iff propertyName exists in component.
	//   Property names are case insensitive, to give more flexibility for the few property names.
	bool component_hasProperty(const Component& component, const ReadableString& propertyName);
	// Sets a property found using propertyName in component to the value serialized in value.
	//   Raises an exception if component doesn't exist.
	//   Matching of propertyName is case insensitive, to give more flexibility for the few property names.
	//   Returns ReturnCode::Good if assigned.
	//   Unless mustAssign forces an exception.
	//     Returns ReturnCode::KeyNotFound if propertyName wasn't found in component.
	//   Unless mustAssign forces an exception.
	//     Returns ReturnCode::ParsingFailure if propertyName was found but value couldn't be converted to its type.
	ReturnCode component_setProperty(const Component& component, const ReadableString& propertyName, const ReadableString& value, bool mustAssign = true);
	// An advanced overload allowing loading of resources from a specific path using the fromPath argument.
	ReturnCode component_setProperty(const Component& component, const ReadableString& propertyName, const ReadableString& value, const ReadableString& fromPath, bool mustAssign = true);
	// A version for setting integers and booleans.
	//   For integers:
	//     Just set value to whatever you want assigned directly.
	//     This is faster than using component_setProperty by not parsing the value from any string.
	//   For booleans:
	//     Use 1 (or another non-zero value) for true and 0 for false.
	//     Boolean properties will convery any non-zero values into ones.
	ReturnCode component_setProperty_integer(const Component& component, const ReadableString& propertyName, int64_t value, bool mustAssign = true);
	// A version for setting images.
	ReturnCode component_setProperty_image(const Component& component, const ReadableString& propertyName, const OrderedImageRgbaU8& value, bool mustAssign = true);
	// A version optimized for basic strings to bypass quote mangling
	//   The new value is given as it is without unmangling.
	ReturnCode component_setProperty_string(const Component& component, const ReadableString& propertyName, const ReadableString& value, bool mustAssign = true);
	// Returns a property found using propertyName in component.
	//   Raises an exception if component doesn't exist.
	//   Matching of propertyName is case insensitive, to give more flexibility for the few property names.
	//   If mustExist is true
	//     Raises an exception when propertyName isn't found.
	//   If mustExist is false
	//     Returns an empty string when propertyName isn't found.
	String component_getProperty(const Component& component, const ReadableString& propertyName, bool mustExist = true);
	// A version for getting integers and booleans.
	//   Returns defaultValue on failure, which should use a value that is never actually used.
	int64_t component_getProperty_integer(const Component& component, const ReadableString& propertyName, bool mustExist = true, int64_t defaultValue = 0);
	// A version for getting images
	OrderedImageRgbaU8 component_getProperty_image(const Component& component, const ReadableString& propertyName, bool mustExist = true);
	// A version optimized for basic strings to bypass quote mangling.
	//   Returns the result without adding any quote signs or escape characters.
	String component_getProperty_string(const Component& component, const ReadableString& propertyName, bool mustExist = true);

	// Call a named method in the component using optional text arguments.
	//   Matching of methodName is case insensitive, to give more flexibility for the few method names.
	String component_call(const Component& component, const ReadableString& methodName);
	String component_call(const Component& component, const ReadableString& methodName, const ReadableString& arguments);

// Component events
	// The main activation of clickable components.
	//   The pressed callback doesn't take any arguments, because it should be possible to generate from multiple input methods.
	void component_setPressedEvent(const Component& component, const EmptyCallback& event);
	// Called before the component and it's child components are destructed.
	// The event is called when there are no more handles to component.
	//   Be careful not to forget any extra handles to the component if a memory leak would significantly change the behavior.
	void component_setDestroyEvent(const Component& component, const EmptyCallback& event);
	// Mouse-down activates when any mouse button is pressed down within the component
	//   Raises an exception if component doesn't exist.
	//   The component itself decides if the mouse is inside, which allow rounded components to act as their true shape.
	void component_setMouseDownEvent(const Component& component, const MouseCallback& mouseEvent);
	// Mouse-up should eventually follow after a mouse-down event, to ensure basic transaction safety.
	//   Raises an exception if component doesn't exist.
	//   * Even if the mouse is dragged outside of the component or window before being lifted.
	//   * Even if the component is removed from the window while the button is pressed,
	//     the button press will keep it alive long enough to receive the mouse-up event before being freed.
	void component_setMouseUpEvent(const Component& component, const MouseCallback& mouseEvent);
	// Mouse-move is triggered when the mouse moves over the component.
	//   Raises an exception if component doesn't exist.
	//   * When pressed down inside of the component, dragging outside the component or even window will
	//     continue to give mouse-move events to the callback.
	//   * If dragging left of or above the window, event.position may contain negative coordinates.
	void component_setMouseMoveEvent(const Component& component, const MouseCallback& mouseEvent);
	// Mouse-scroll is triggered by scrolling in any direction.
	//   Raises an exception if component doesn't exist.
	//   Currently only supporting MouseKeyEnum::ScrollUp and MouseKeyEnum::ScrollDown as values in event.key.
	void component_setMouseScrollEvent(const Component& component, const MouseCallback& mouseEvent);
	// Key-down only comes when a button is pressed down. (No repeat)
	//   Raises an exception if component doesn't exist.
	//   The backend window is responsible to filter away any false positives for down events caused by repetition.
	void component_setKeyDownEvent(const Component& component, const KeyboardCallback& keyboardEvent);
	// Key-up only comes when a button is lifted after being pressed. (No repeat)
	//   Raises an exception if component doesn't exist.
	void component_setKeyUpEvent(const Component& component, const KeyboardCallback& keyboardEvent);
	// Key-type comes both when a key is pressed, and then repeatedly without having to lift the key.
	//   Raises an exception if component doesn't exist.
	//   There's usually a second's delay before quickly repeating.
	void component_setKeyTypeEvent(const Component& component, const KeyboardCallback& keyboardEvent);
	// Select events are sent when the selected index of something has changed.
	//   Used in Listbox to cover updates from all different ways the selection may change.
	void component_setSelectEvent(const Component& component, const IndexCallback& selectEvent);

// Theme
	// Apply the given theme recursively to all components in the window's interface.
	//   Raises an exception if window or component doesn't exist.
	//   Components will gather what they can from the theme and save it for later.
	//   Changing a theme while being used by an interface or adding new components,
	//     should apply the theme again to ensure that all changes are applied.
	//     TODO: Automate this process by storing a reference to the theme in each component and checking for updates before drawing.
	void window_applyTheme(const Window& window, const VisualTheme& theme);

}

#endif

