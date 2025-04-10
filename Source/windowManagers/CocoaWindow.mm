
#import <Cocoa/Cocoa.h>

#include "../DFPSR/api/imageAPI.h"
#include "../DFPSR/api/drawAPI.h"
#include "../DFPSR/api/timeAPI.h"
#include "../DFPSR/implementation/gui/BackendWindow.h"
#include "../DFPSR/base/heap.h"
#include <climits>

#include "../DFPSR/settings.h"

// Cocoa can only be called from the main thread.
//   So it is not even possible to have a dedicated background thread for managing the window.
//   No multi-threading at all can be used for Cocoa.

static const int bufferCount = 2;

static bool applicationInitialized = false;
static NSApplication *application;

class CocoaWindow : public dsr::BackendWindow {
private:
	// Handle to the Cocoa window
	NSWindow *window = nullptr;
	// Last modifiers to allow converting NSEventTypeFlagsChanged into up and down key press events.
	bool pressedControlCommand = false;
	bool pressedShift = false;
	bool pressedAltOption = false;

	// Double buffering to allow drawing to a canvas while displaying the previous one
	// The image which can be drawn to, sharing memory with the Cocoa image
	dsr::AlignedImageRgbaU8 canvas[bufferCount];
	// An Cocoa image wrapped around the canvas pixel data
	//????Image *canvasX[bufferCount] = {};
	int drawIndex = 0 % bufferCount;
	int showIndex = 1 % bufferCount;

	// Remembers the dimensions of the window from creation and resize events
	//   This allow requesting the size of the window at any time
	int windowWidth = 0, windowHeight = 0;

	// Called before the application fetches events from the input queue
	//   Closing the window, moving the mouse, pressing a key, et cetera
	void prefetchEvents() override;
	/*
	// Called to change the cursor visibility and returning true on success
	void applyCursorVisibility();
	bool setCursorVisibility(bool visible) override;

	// Place the cursor within the window
	void setCursorPosition(int x, int y) override;

	// Color format
	dsr::PackOrderIndex packOrderIndex = dsr::PackOrderIndex::RGBA;
	dsr::PackOrderIndex getColorFormat();
	*/
private:
	// Helper methods specific to calling XLib
	void updateTitle();
private:
	// Canvas methods
	dsr::AlignedImageRgbaU8 getCanvas() override { return this->canvas[this->drawIndex]; }
	void resizeCanvas(int width, int height) override;
	// Window methods
	void setTitle(const dsr::String &newTitle) override {
		this->title = newTitle;
		this->updateTitle();
	}
	int windowState = 0; // 0=none, 1=windowed, 2=fullscreen
public:
	// Constructors
	CocoaWindow(const CocoaWindow&) = delete; // Non-copyable because of pointer aliasing.
	CocoaWindow(const dsr::String& title, int width, int height);
	int getWidth() const override { return this->windowWidth; };
	int getHeight() const override { return this->windowHeight; };
	// Destructor
	~CocoaWindow();
	// Full-screen
	void setFullScreen(bool enabled) override {
		// TODO: Implement full-screen.
	};
	bool isFullScreen() override { return this->windowState == 2; }
	// Showing the content
	void showCanvas() override;

	// TODO: Implement clipboard access.
	//dsr::ReadableString loadFromClipboard(double timeoutInSeconds) override;
	//void saveToClipboard(const dsr::ReadableString &text, double timeoutInSeconds) override;
};

void CocoaWindow::updateTitle() {
	// Encode the title string as null terminated UFT-8.
	dsr::Buffer utf8_title = dsr::string_saveToMemory(this->title, dsr::CharacterEncoding::BOM_UTF8, dsr::LineEncoding::Lf, false, true);
	// Create a native string for MacOS.
	NSString *windowTitle = [NSString stringWithUTF8String:(char *)(dsr::buffer_dangerous_getUnsafeData(utf8_title))];
	// Set the window title.
	[window setTitle:windowTitle];
}

CocoaWindow::CocoaWindow(const dsr::String& title, int width, int height) {
	if (!applicationInitialized) {
		application = [NSApplication sharedApplication];
		[application setActivationPolicy:NSApplicationActivationPolicyRegular];
		[application setPresentationOptions:NSApplicationPresentationDefault];
		[application activateIgnoringOtherApps:YES];
		applicationInitialized = true;
	}
	bool fullScreen = false;
	if (width < 1 || height < 1) {
		fullScreen = true;
		width = 400;
		height = 300;
	}

	NSRect region = NSMakeRect(0, 0, width, height);
	// Create a window
	@autoreleasepool {
		this->window = [[NSWindow alloc]
		  initWithContentRect:region
		  styleMask: (
			  NSWindowStyleMaskTitled
			| NSWindowStyleMaskClosable
			| NSWindowStyleMaskMiniaturizable
			| NSWindowStyleMaskResizable
		  )
		  backing: NSBackingStoreBuffered
		  defer: NO];
	}

	// Set the title
	this->setTitle(title);
	// Show the window.
	[window center];
	[window makeKeyAndOrderFront:nil];
	[window makeFirstResponder:nil];
}

// Also locked, but cannot change the name when overriding
void CocoaWindow::prefetchEvents() {
	@autoreleasepool {
		// Process events
		while (true) {
			NSEvent *event = [application nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
			if (event == nullptr) break;
			if ([event type] == NSEventTypeLeftMouseDown
			 || [event type] == NSEventTypeLeftMouseDragged
			 || [event type] == NSEventTypeLeftMouseUp
			 || [event type] == NSEventTypeRightMouseDown
			 || [event type] == NSEventTypeRightMouseDragged
			 || [event type] == NSEventTypeRightMouseUp
			 || [event type] == NSEventTypeOtherMouseDown
			 || [event type] == NSEventTypeOtherMouseDragged
			 || [event type] == NSEventTypeOtherMouseUp
			 || [event type] == NSEventTypeMouseMoved
			 || [event type] == NSEventTypeMouseEntered
			 || [event type] == NSEventTypeMouseExited
			 || [event type] == NSEventTypeScrollWheel) {
				NSView *view = [window contentView];
				CGFloat canvasHeight = NSHeight(view.bounds);
				NSPoint point = [view convertPoint:[event locationInWindow] fromView:nil];
				// This nasty hack combines an old mouse event with a canvas size that may have changed since the mouse event was created.
				// TODO: Find a way to get the canvas height from when the mouse event was actually created, so that lagging while resizing a window can not place click events at the wrong coordiates.
				dsr::IVector2D mousePosition = dsr::IVector2D(int32_t(point.x), int32_t(canvasHeight - point.y));
				if ([event type] == NSEventTypeLeftMouseDown) {
					dsr::printText(U"LeftMouseDown at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeLeftMouseDragged) {
					dsr::printText(U"LeftMouseDragged at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeLeftMouseUp) {
					dsr::printText(U"LeftMouseUp at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeRightMouseDown) {
					dsr::printText(U"RightMouseDown at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeRightMouseDragged) {
					dsr::printText(U"RightMouseDragged at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeRightMouseUp) {
					dsr::printText(U"RightMouseUp at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeOtherMouseDown) {
					dsr::printText(U"OtherMouseDown at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeOtherMouseDragged) {
					dsr::printText(U"OtherMouseDragged at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeOtherMouseUp) {
					dsr::printText(U"OtherMouseUp at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeMouseMoved) {
					dsr::printText(U"MouseMoved at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeMouseEntered) {
					dsr::printText(U"MouseEntered at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeMouseExited) {
					dsr::printText(U"MouseExited at ", mousePosition, U"\n");
				} else if ([event type] == NSEventTypeScrollWheel) {
					dsr::printText(U"ScrollWheel at ", mousePosition, U"\n");
				}
				[this->window makeKeyAndOrderFront:nil];
			} else if ([event type] == NSEventTypeKeyDown
			        || [event type] == NSEventTypeKeyUp
			        || [event type] == NSEventTypeFlagsChanged) {
				// TODO: Make sure that the window catches keyboard events instead of corrupting terminal input.
				if ([event type] == NSEventTypeKeyDown) {
					if (!(event.isARepeat)) {
						dsr::printText(U"KeyDown: keyCode = ", event.keyCode, U"\n");
					}
					dsr::printText(U"KeyType\n");
				} else if ([event type] == NSEventTypeKeyUp) {
					dsr::printText(U"KeyUp: keyCode = ", event.keyCode, U"\n");
				} else if ([event type] == NSEventTypeFlagsChanged) {
					dsr::printText(U"FlagsChanged\n");
					NSEventModifierFlags newModifierFlags = [event modifierFlags];
					bool newControlCommand = (newModifierFlags & (NSEventModifierFlagControl | NSEventModifierFlagCommand)) != 0u;
					bool newShift = (newModifierFlags & NSEventModifierFlagShift) != 0u;
					bool newAltOption = (newModifierFlags & NSEventModifierFlagOption) != 0u;
					if (newControlCommand && !pressedControlCommand) {
						dsr::printText(U"KeyDown: Control\n");
					} else if (!newControlCommand && pressedControlCommand) {
						dsr::printText(U"KeyUp: Control\n");
					}
					if (newShift && !pressedShift) {
						dsr::printText(U"KeyDown: Shift\n");
					} else if (!newShift && pressedShift) {
						dsr::printText(U"KeyUp: Shift\n");
					}
					if (newAltOption && !pressedAltOption) {
						dsr::printText(U"KeyDown: Alt\n");
					} else if (!newAltOption && pressedAltOption) {
						dsr::printText(U"KeyUp: Alt\n");
					}
					pressedControlCommand = newControlCommand;
					pressedShift = newShift;
					pressedAltOption = newAltOption;
				}
			}
			[application sendEvent:event];
			[application updateWindows];
		}
	}
}

// Locked because it overrides
void CocoaWindow::resizeCanvas(int width, int height) {
	// TODO: Resize.
}

CocoaWindow::~CocoaWindow() {
	[this->window close];
	window = nullptr;
}

void CocoaWindow::showCanvas() {
	// TODO: Implement
}

dsr::Handle<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	return dsr::handle_create<CocoaWindow>(title, width, height);
}
