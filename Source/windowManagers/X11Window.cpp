﻿
// Avoid cluttering the global namespace by hiding these from the header
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#include "../DFPSR/api/imageAPI.h"
#include "../DFPSR/api/drawAPI.h"
#include "../DFPSR/api/timeAPI.h"
#include "../DFPSR/implementation/gui/BackendWindow.h"
#include "../DFPSR/base/heap.h"
#include <climits>

#include "../DFPSR/settings.h"

#ifndef DISABLE_MULTI_THREADING
	// According to this documentation, XInitThreads doesn't have to be used if a mutex is wrapped around all the calls to XLib.
	//   https://tronche.com/gui/x/xlib/display/XInitThreads.html
	#include <mutex>
	#include <future>

	static std::mutex windowLock;
	inline void lockWindow() { windowLock.lock(); }
	inline void unlockWindow() { windowLock.unlock(); }
#else
	inline void lockWindow() {}
	inline void unlockWindow() {}
#endif

static const int bufferCount = 2;

class X11Window : public dsr::BackendWindow {
private:
	// The display is the connection to the X server
	//   Each window has it's own connection, because you're only supposed to have one window
	//   Let sub-windows be visual components for simpler input and deterministic custom decorations
	Display *display = nullptr;
	// The handle to the X11 window
	Window window;
	// Holds settings for drawing to the window
	GC graphicsContext;
	// Invisible cursor for hiding it over the window
	Cursor noCursor;

	// Double buffering to allow drawing to a canvas while displaying the previous one
	// The image which can be drawn to, sharing memory with the X11 image
	dsr::AlignedImageRgbaU8 canvas[bufferCount];
	// An X11 image wrapped around the canvas pixel data
	XImage *canvasX[bufferCount] = {};
	int drawIndex = 0 % bufferCount;
	int showIndex = 1 % bufferCount;
	bool firstFrame = true;

	#ifndef DISABLE_MULTI_THREADING
		// The background worker for displaying the result using a separate thread protected by a mutex
		std::future<void> displayFuture;
	#endif

	// Remembers the dimensions of the window from creation and resize events
	//   This allow requesting the size of the window at any time
	int windowWidth = 0, windowHeight = 0;

	// Called before the application fetches events from the input queue
	//   Closing the window, moving the mouse, pressing a key, et cetera
	void prefetchEvents() override;

	// Called to change the cursor visibility and returning true on success
	void applyCursorVisibility_locked();
	bool setCursorVisibility(bool visible) override;

	// Place the cursor within the window
	bool setCursorPosition(int x, int y) override;

	// Color format
	dsr::PackOrderIndex packOrderIndex = dsr::PackOrderIndex::RGBA;
	dsr::PackOrderIndex getColorFormat_locked();
private:
	// Helper methods specific to calling XLib
	void updateTitle_locked();
private:
	// Canvas methods
	dsr::AlignedImageRgbaU8 getCanvas() override { return this->canvas[this->drawIndex]; }
	void resizeCanvas(int width, int height) override;
	// Window methods
	void setTitle(const dsr::String &newTitle) override {
		this->title = newTitle;
		this->updateTitle_locked();
	}
	void removeOldWindow_locked();
	void createGCWindow_locked(const dsr::String& title, int width, int height);
	void createWindowed_locked(const dsr::String& title, int width, int height);
	void createFullscreen_locked();
	void prepareWindow_locked();
	int windowState = 0; // 0=none, 1=windowed, 2=fullscreen
public:
	// Constructors
	X11Window(const X11Window&) = delete; // Non-copyable because of pointer aliasing.
	X11Window(const dsr::String& title, int width, int height);
	int getWidth() const override { return this->windowWidth; };
	int getHeight() const override { return this->windowHeight; };
	// Destructor
	~X11Window();
	// Full-screen
	void setFullScreen(bool enabled) override;
	bool isFullScreen() override { return this->windowState == 2; }
	// Showing the content
	void showCanvas() override;
	// Clipboard access
	Atom clipboardAtom, targetsAtom, utf8StringAtom, targetAtom;
	bool loadingFromClipboard = false;
	dsr::String textFromClipboard;
	dsr::String textToClipboard;
	void listContentInClipboard();
	void initializeClipboard();
	void terminateClipboard();
	dsr::ReadableString loadFromClipboard(double timeoutInSeconds) override;
	void saveToClipboard(const dsr::ReadableString &text, double timeoutInSeconds) override;
};

void X11Window::initializeClipboard() {
	this->clipboardAtom = XInternAtom(this->display , "CLIPBOARD", False);
	this->targetsAtom = XInternAtom(this->display , "TARGETS", False);
	this->utf8StringAtom = XInternAtom(this->display, "UTF8_STRING", False);
	this->targetAtom = None;
}

void X11Window::terminateClipboard() {
	// TODO: Send ownership to the clipboard to allow pasting after terminating the program it was copied from.
}

dsr::ReadableString X11Window::loadFromClipboard(double timeoutInSeconds) {
	// The timeout needs to be at least 10 milliseconds to give it a fair chance.
	if (timeoutInSeconds < 0.01) timeoutInSeconds = 0.01;
	// Request text to paste and wait some time for an application to respond.
	XConvertSelection(this->display, this->clipboardAtom, this->targetsAtom, this->clipboardAtom, this->window, CurrentTime);
	this->loadingFromClipboard = true;
	double deadline = dsr::time_getSeconds() + timeoutInSeconds;
	while (this->loadingFromClipboard && dsr::time_getSeconds() < deadline) {
		this->prefetchEvents();
		dsr::time_sleepSeconds(0.001);
	}
	return this->loadingFromClipboard ? U"" : this->textFromClipboard;
}

void X11Window::listContentInClipboard() {
	// Tell other programs sharing the clipboard that something is available to paste.
	XSetSelectionOwner(this->display, this->clipboardAtom, this->window, CurrentTime);
}

void X11Window::saveToClipboard(const dsr::ReadableString &text, double timeoutInSeconds) {
	this->textToClipboard = text;
	this->listContentInClipboard();
}

bool X11Window::setCursorPosition(int x, int y) {
	lockWindow();
		XWarpPointer(this->display, this->window, this->window, 0, 0, this->windowWidth, this->windowHeight, x, y);
	unlockWindow();
	return true;
}

void X11Window::applyCursorVisibility_locked() {
	lockWindow();
		if (this->visibleCursor) {
			// Reset to parent cursor
			XUndefineCursor(this->display, this->window);
		} else {
			// Let the window display an empty cursor
			XDefineCursor(this->display, this->window, this->noCursor);
		}
	unlockWindow();
}

bool X11Window::setCursorVisibility(bool visible) {
	// Remember the cursor's visibility for anyone asking
	this->visibleCursor = visible;
	// Use the stored visibility to update the cursor
	this->applyCursorVisibility_locked();
	// Indicate success
	return true;
}

void X11Window::updateTitle_locked() {
	lockWindow();
		XSetStandardProperties(this->display, this->window, dsr::FixedAscii<512>(this->title).getPointer(), "Icon", None, NULL, 0, NULL);
	unlockWindow();
}

dsr::PackOrderIndex X11Window::getColorFormat_locked() {
	lockWindow();
		XVisualInfo visualRequest;
		visualRequest.screen = 0;
		visualRequest.depth = 32;
		visualRequest.c_class = TrueColor;
		int visualCount;
		dsr::PackOrderIndex result = dsr::PackOrderIndex::RGBA;
		XVisualInfo *formatList = XGetVisualInfo(this->display, VisualScreenMask | VisualDepthMask | VisualClassMask, &visualRequest, &visualCount);
		if (formatList != nullptr) {
			for (int i = 0; i < visualCount; i++) {
				if (formatList[i].bits_per_rgb == 8) {
					const uint32_t red = formatList[i].red_mask;
					const uint32_t green = formatList[i].green_mask;
					const uint32_t blue = formatList[i].blue_mask;
					const uint32_t first = 255u;
					const uint32_t second = 255u << 8u;
					const uint32_t third = 255u << 16u;
					const uint32_t fourth = 255u << 24u;
					if (red == first && green == second && blue == third) {
						result = dsr::PackOrderIndex::RGBA;
					} else if (red == second && green == third && blue == fourth) {
						result = dsr::PackOrderIndex::ARGB;
					} else if (blue == first && green == second && red == third) {
						result = dsr::PackOrderIndex::BGRA;
					} else if (blue == second && green == third && red == fourth) {
						result = dsr::PackOrderIndex::ABGR;
					} else {
						dsr::throwError(U"Error! Unhandled 32-bit color format. Only RGBA, ARGB, BGRA and ABGR are currently supported.\n");
					}
					break;
				}
			}
			XFree(formatList);
		} else {
			visualRequest.depth = 24;
			XVisualInfo *formatList = XGetVisualInfo(this->display, VisualScreenMask | VisualDepthMask | VisualClassMask, &visualRequest, &visualCount);
			if (formatList != nullptr) {
				for (int i = 0; i < visualCount; i++) {
					if (formatList[i].bits_per_rgb == 8) {
						const uint32_t red = formatList[i].red_mask;
						const uint32_t green = formatList[i].green_mask;
						const uint32_t blue = formatList[i].blue_mask;
						const uint32_t first = 255u;
						const uint32_t second = 255u << 8u;
						const uint32_t third = 255u << 16u;
						if (red == first && green == second && blue == third) {
							result = dsr::PackOrderIndex::RGBA; // RGB
						} else if (blue == first && green == second && red == third) {
							result = dsr::PackOrderIndex::BGRA; // BGR
						} else {
							dsr::throwError(U"Error! Unhandled 24-bit color format. Only RGB and BGR are currently supported.\n");
						}
						break;
					}
				}
				XFree(formatList);
			} else {
				dsr::throwError(U"Error! The display does not support any known 24 truecolor formats.\n");
			}
		}
	unlockWindow();
	return result;
}

struct Hints {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long          inputMode;
	unsigned long status;
};

void X11Window::setFullScreen(bool enabled) {
	if (this->windowState == 1 && enabled) {
		// Clean up any previous X11 window
		removeOldWindow_locked();
		// Create the new window and graphics context
		this->createFullscreen_locked();
	} else if (this->windowState == 2 && !enabled) {
		// Clean up any previous X11 window
		removeOldWindow_locked();
		// Create the new window and graphics context
		this->createWindowed_locked(this->title, 800, 600); // TODO: Remember the dimensions from last windowed mode
	}
	this->applyCursorVisibility_locked();
	lockWindow();
		listContentInClipboard();
	unlockWindow();
}

void X11Window::removeOldWindow_locked() {
	lockWindow();
		if (this->windowState != 0) {
			XFreeGC(this->display, this->graphicsContext);
			XDestroyWindow(this->display, this->window);
			XUngrabPointer(this->display, CurrentTime);
		}
		this->windowState = 0;
	unlockWindow();
}

void X11Window::prepareWindow_locked() {
	lockWindow();
		// Set input masks
		XSelectInput(this->display, this->window,
		  ExposureMask | StructureNotifyMask |
		  PointerMotionMask |
		  ButtonPressMask | ButtonReleaseMask |
		  KeyPressMask | KeyReleaseMask
		);

		// Listen to the window close event.
		Atom WM_DELETE_WINDOW = XInternAtom(this->display, "WM_DELETE_WINDOW", False);
		XSetWMProtocols(this->display, this->window, &WM_DELETE_WINDOW, 1);
	unlockWindow();
	// Reallocate the canvas
	this->resizeCanvas(this->windowWidth, this->windowHeight);
}

void X11Window::createGCWindow_locked(const dsr::String& title, int width, int height) {
	lockWindow();
		// Request to resize the canvas and interface according to the new window
		this->windowWidth = width;
		this->windowHeight = height;
		this->receivedWindowResize(width, height);
		int screenIndex = DefaultScreen(this->display);
		unsigned long black = BlackPixel(this->display, screenIndex);
		unsigned long white = WhitePixel(this->display, screenIndex);
		// Create a new window
		this->window = XCreateSimpleWindow(this->display, DefaultRootWindow(this->display), 0, 0, width, height, 0, white, black);
	unlockWindow();

	this->updateTitle_locked();

	lockWindow();
		// Create a new graphics context
		this->graphicsContext = XCreateGC(this->display, this->window, 0, 0);
		XSetBackground(this->display, this->graphicsContext, black);
		XSetForeground(this->display, this->graphicsContext, white);
		XClearWindow(this->display, this->window);
	unlockWindow();
}

void X11Window::createWindowed_locked(const dsr::String& title, int width, int height) {
	// Create the window
	this->createGCWindow_locked(title, width, height);
	lockWindow();
		// Display the window when done placing it
		XMapRaised(this->display, this->window);

		this->windowState = 1;
		this->firstFrame = true;
	unlockWindow();
	this->prepareWindow_locked();
}

void X11Window::createFullscreen_locked() {
	lockWindow();
		// Get the screen resolution
		Screen* screenInfo = DefaultScreenOfDisplay(this->display);
	unlockWindow();

	// Create the window
	this->createGCWindow_locked(U"", screenInfo->width, screenInfo->height);

	lockWindow();
		// Override redirect
		unsigned long valuemask = CWOverrideRedirect;
		XSetWindowAttributes setwinattr;
		setwinattr.override_redirect = 1;
		XChangeWindowAttributes(this->display, this->window, valuemask, &setwinattr);

		// Remove decorations
		Hints hints;
		memset(&hints, 0, sizeof(hints));
		hints.flags = 2;
		hints.decorations = 0;
		Atom property;
		property = XInternAtom(this->display, "_MOTIF_WM_HINTS", True); // TODO: Check if this optional XLib feature is supported
		XChangeProperty(this->display, this->window, property, property, 32, PropModeReplace, (unsigned char*)&hints, 5);

		// Move to absolute origin
		XMoveResizeWindow(this->display, this->window, 0, 0, screenInfo->width, screenInfo->height);

		// Prevent accessing anything outside of the window until it closes (for multiple displays)
		XGrabPointer(this->display, this->window, 1, 0, GrabModeAsync, GrabModeAsync, this->window, 0L, CurrentTime);
		XGrabKeyboard(this->display, this->window, 1, GrabModeAsync, GrabModeAsync, CurrentTime);

		// Display the window when done placing it
		XMapRaised(this->display, this->window);

		// Now that the window is visible, it can be focused for keyboard input
		XSetInputFocus(this->display, this->window, RevertToNone, CurrentTime);

		this->windowState = 2;
		this->firstFrame = true;
	unlockWindow();
	this->prepareWindow_locked();
}

X11Window::X11Window(const dsr::String& title, int width, int height) {
	bool fullScreen = false;
	if (width < 1 || height < 1) {
		fullScreen = true;
		width = 400;
		height = 300;
	}

	lockWindow();
		this->display = XOpenDisplay(nullptr);
	unlockWindow();
	if (this->display == nullptr) {
		dsr::throwError(U"Error! Failed to open XLib display!\n");
		return;
	}

	// Get the color format
	this->packOrderIndex = this->getColorFormat_locked();

	// Remember the title
	this->title = title;

	// Create a window
	if (fullScreen) {
		this->createFullscreen_locked();
	} else {
		this->createWindowed_locked(title, width, height);
	}

	// Create a hidden cursor stored as noCursor
	// Create a black color using zero bits, which will not be visible anyway
	XColor black; memset(&black, 0, sizeof(XColor));
	// Store all 8x8 pixels in a 64-bit unsigned integer
	uint64_t zeroBits = 0u;
	// Create a temporary image for both 1-bit color selection and a visibility mask
	Pixmap zeroBitmap = XCreateBitmapFromData(this->display, this->window, (char*)&zeroBits, 8, 8);
	// Create the cursor
	this->noCursor = XCreatePixmapCursor(this->display, zeroBitmap, zeroBitmap, &black, &black, 0, 0);
	// Free the temporary bitmap used to create the cursor
	XFreePixmap(this->display, zeroBitmap);

	// Create things needed for copying and pasting text.
	this->initializeClipboard();
}

// Convert keycodes from XLib to DSR
static dsr::MouseKeyEnum getMouseKey(int keyCode) {
	dsr::MouseKeyEnum result = dsr::MouseKeyEnum::NoKey;
	if (keyCode == Button1) {
		result = dsr::MouseKeyEnum::Left;
	} else if (keyCode == Button2) {
		result = dsr::MouseKeyEnum::Middle;
	} else if (keyCode == Button3) {
		result = dsr::MouseKeyEnum::Right;
	} else if (keyCode == Button4) {
		result = dsr::MouseKeyEnum::ScrollUp;
	} else if (keyCode == Button5) {
		result = dsr::MouseKeyEnum::ScrollDown;
	}
	return result;
}

static bool isVerticalScrollKey(dsr::MouseKeyEnum key) {
	return key == dsr::MouseKeyEnum::ScrollDown || key == dsr::MouseKeyEnum::ScrollUp;
}

static dsr::DsrKey getDsrKey(KeySym keyCode) {
	dsr::DsrKey result = dsr::DsrKey_Unhandled;
	if (keyCode == XK_Escape) {
		result = dsr::DsrKey_Escape;
	} else if (keyCode == XK_F1) {
		result = dsr::DsrKey_F1;
	} else if (keyCode == XK_F2) {
		result = dsr::DsrKey_F2;
	} else if (keyCode == XK_F3) {
		result = dsr::DsrKey_F3;
	} else if (keyCode == XK_F4) {
		result = dsr::DsrKey_F4;
	} else if (keyCode == XK_F5) {
		result = dsr::DsrKey_F5;
	} else if (keyCode == XK_F6) {
		result = dsr::DsrKey_F6;
	} else if (keyCode == XK_F7) {
		result = dsr::DsrKey_F7;
	} else if (keyCode == XK_F8) {
		result = dsr::DsrKey_F8;
	} else if (keyCode == XK_F9) {
		result = dsr::DsrKey_F9;
	} else if (keyCode == XK_F10) {
		result = dsr::DsrKey_F10;
	} else if (keyCode == XK_F11) {
		result = dsr::DsrKey_F11;
	} else if (keyCode == XK_F12) {
		result = dsr::DsrKey_F12;
	} else if (keyCode == XK_Pause) {
		result = dsr::DsrKey_Pause;
	} else if (keyCode == XK_space) {
		result = dsr::DsrKey_Space;
	} else if (keyCode == XK_Tab) {
		result = dsr::DsrKey_Tab;
	} else if (keyCode == XK_Return) {
		result = dsr::DsrKey_Return;
	} else if (keyCode == XK_BackSpace) {
		result = dsr::DsrKey_BackSpace;
	} else if (keyCode == XK_Shift_L || keyCode == XK_Shift_R) {
		result = dsr::DsrKey_Shift;
	} else if (keyCode == XK_Control_L || keyCode == XK_Control_R) {
		result = dsr::DsrKey_Control;
	} else if (keyCode == XK_Alt_L || keyCode == XK_Alt_R) {
		result = dsr::DsrKey_Alt;
	} else if (keyCode == XK_Delete) {
		result = dsr::DsrKey_Delete;
	} else if (keyCode == XK_Left) {
		result = dsr::DsrKey_LeftArrow;
	} else if (keyCode == XK_Right) {
		result = dsr::DsrKey_RightArrow;
	} else if (keyCode == XK_Up) {
		result = dsr::DsrKey_UpArrow;
	} else if (keyCode == XK_Down) {
		result = dsr::DsrKey_DownArrow;
	} else if (keyCode == XK_0) {
		result = dsr::DsrKey_0;
	} else if (keyCode == XK_1) {
		result = dsr::DsrKey_1;
	} else if (keyCode == XK_2) {
		result = dsr::DsrKey_2;
	} else if (keyCode == XK_3) {
		result = dsr::DsrKey_3;
	} else if (keyCode == XK_4) {
		result = dsr::DsrKey_4;
	} else if (keyCode == XK_5) {
		result = dsr::DsrKey_5;
	} else if (keyCode == XK_6) {
		result = dsr::DsrKey_6;
	} else if (keyCode == XK_7) {
		result = dsr::DsrKey_7;
	} else if (keyCode == XK_8) {
		result = dsr::DsrKey_8;
	} else if (keyCode == XK_9) {
		result = dsr::DsrKey_9;
	} else if (keyCode == XK_a || keyCode == XK_A) {
		result = dsr::DsrKey_A;
	} else if (keyCode == XK_b || keyCode == XK_B) {
		result = dsr::DsrKey_B;
	} else if (keyCode == XK_c || keyCode == XK_C) {
		result = dsr::DsrKey_C;
	} else if (keyCode == XK_d || keyCode == XK_D) {
		result = dsr::DsrKey_D;
	} else if (keyCode == XK_e || keyCode == XK_E) {
		result = dsr::DsrKey_E;
	} else if (keyCode == XK_f || keyCode == XK_F) {
		result = dsr::DsrKey_F;
	} else if (keyCode == XK_g || keyCode == XK_G) {
		result = dsr::DsrKey_G;
	} else if (keyCode == XK_h || keyCode == XK_H) {
		result = dsr::DsrKey_H;
	} else if (keyCode == XK_i || keyCode == XK_I) {
		result = dsr::DsrKey_I;
	} else if (keyCode == XK_j || keyCode == XK_J) {
		result = dsr::DsrKey_J;
	} else if (keyCode == XK_k || keyCode == XK_K) {
		result = dsr::DsrKey_K;
	} else if (keyCode == XK_l || keyCode == XK_L) {
		result = dsr::DsrKey_L;
	} else if (keyCode == XK_m || keyCode == XK_M) {
		result = dsr::DsrKey_M;
	} else if (keyCode == XK_n || keyCode == XK_N) {
		result = dsr::DsrKey_N;
	} else if (keyCode == XK_o || keyCode == XK_O) {
		result = dsr::DsrKey_O;
	} else if (keyCode == XK_p || keyCode == XK_P) {
		result = dsr::DsrKey_P;
	} else if (keyCode == XK_q || keyCode == XK_Q) {
		result = dsr::DsrKey_Q;
	} else if (keyCode == XK_r || keyCode == XK_R) {
		result = dsr::DsrKey_R;
	} else if (keyCode == XK_s || keyCode == XK_S) {
		result = dsr::DsrKey_S;
	} else if (keyCode == XK_t || keyCode == XK_T) {
		result = dsr::DsrKey_T;
	} else if (keyCode == XK_u || keyCode == XK_U) {
		result = dsr::DsrKey_U;
	} else if (keyCode == XK_v || keyCode == XK_V) {
		result = dsr::DsrKey_V;
	} else if (keyCode == XK_w || keyCode == XK_W) {
		result = dsr::DsrKey_W;
	} else if (keyCode == XK_x || keyCode == XK_X) {
		result = dsr::DsrKey_X;
	} else if (keyCode == XK_y || keyCode == XK_Y) {
		result = dsr::DsrKey_Y;
	} else if (keyCode == XK_z || keyCode == XK_Z) {
		result = dsr::DsrKey_Z;
	} else if (keyCode == XK_Insert) {
		result = dsr::DsrKey_Insert;
	} else if (keyCode == XK_Home) {
		result = dsr::DsrKey_Home;
	} else if (keyCode == XK_End) {
		result = dsr::DsrKey_End;
	} else if (keyCode == XK_Page_Up) {
		result = dsr::DsrKey_PageUp;
	} else if (keyCode == XK_Page_Down) {
		result = dsr::DsrKey_PageDown;
	}
	return result;
}

static dsr::DsrChar getCharacterCode(XEvent& event) {
	const int buffersize = 8;
	KeySym key; char codePoints[buffersize]; dsr::DsrChar character = '\0';
	if (XLookupString(&event.xkey, codePoints, buffersize, &key, 0) == 1) {
		// X11 does not specify any encoding, but BOM_UTF16LE seems to work on Linux.
		// TODO: See if there is a list of X11 character encodings for different platforms.
		dsr::CharacterEncoding encoding = dsr::CharacterEncoding::BOM_UTF16LE;
		dsr::String characterString = string_dangerous_decodeFromData(codePoints, encoding);
		character = characterString[0];
	}
	return character;
}

// Also locked, but cannot change the name when overriding
void X11Window::prefetchEvents() {
	// Only prefetch new events if nothing else is using the communication link
	if (windowLock.try_lock()) {
		if (this->display) {
			bool hasScrolled = false;
			while (XPending(this->display)) {
				// Ensure that full-screen applications have keyboard focus if interacted with in any way
				if (this->windowState == 2) {
					XSetInputFocus(this->display, this->window, RevertToNone, CurrentTime);
				}
				// Get the current event
				XEvent currentEvent;
				XNextEvent(this->display, &currentEvent);
				// See if there's another event
				XEvent nextEvent;
				bool hasNextEvent = XPending(this->display);
				if (hasNextEvent) {
					XPeekEvent(this->display, &nextEvent);
				}
				if (currentEvent.type == Expose && currentEvent.xexpose.count == 0) {
					// Redraw
					this->receivedWindowRedrawEvent();
				} else if (currentEvent.type == KeyPress || currentEvent.type == KeyRelease) {
					// Key down/up
					dsr::DsrChar character = getCharacterCode(currentEvent);
					KeySym nativeKey = XLookupKeysym(&currentEvent.xkey, 0);
					dsr::DsrKey dsrKey = getDsrKey(nativeKey);
					KeySym nextNativeKey = hasNextEvent ? XLookupKeysym(&nextEvent.xkey, 0) : 0;
					// Distinguish between fake and physical repeats using time stamps
					if (hasNextEvent
					 && currentEvent.type == KeyRelease && nextEvent.type == KeyPress
					 && currentEvent.xkey.time == nextEvent.xkey.time
					 && nativeKey == nextNativeKey) {
						// Repeated typing
						this->receivedKeyboardEvent(dsr::KeyboardEventType::KeyType, character, dsrKey);
						// Skip next event
						XNextEvent(this->display, &currentEvent);
					} else {
						if (currentEvent.type == KeyPress) {
							// Physical key down
							this->receivedKeyboardEvent(dsr::KeyboardEventType::KeyDown, character, dsrKey);
							// First press typing
							this->receivedKeyboardEvent(dsr::KeyboardEventType::KeyType, character, dsrKey);
						} else { // currentEvent.type == KeyRelease
							// Physical key up
							this->receivedKeyboardEvent(dsr::KeyboardEventType::KeyUp, character, dsrKey);
						}
					}
				} else if (currentEvent.type == ButtonPress || currentEvent.type == ButtonRelease) {
					dsr::MouseKeyEnum key = getMouseKey(currentEvent.xbutton.button);
					if (isVerticalScrollKey(key)) {
						// Scroll down/up
						if (!hasScrolled) {
							this->receivedMouseEvent(dsr::MouseEventType::Scroll, key, dsr::IVector2D(currentEvent.xbutton.x, currentEvent.xbutton.y));
						}
						hasScrolled = true;
					} else {
						// Mouse down/up
						this->receivedMouseEvent(currentEvent.type == ButtonPress ? dsr::MouseEventType::MouseDown : dsr::MouseEventType::MouseUp, key, dsr::IVector2D(currentEvent.xbutton.x, currentEvent.xbutton.y));
					}
				} else if (currentEvent.type == MotionNotify) {
					// Mouse move
					this->receivedMouseEvent(dsr::MouseEventType::MouseMove, dsr::MouseKeyEnum::NoKey, dsr::IVector2D(currentEvent.xmotion.x, currentEvent.xmotion.y));
				} else if (currentEvent.type == ClientMessage) {
					// Close
					//   Assume WM_DELETE_WINDOW since it is the only registered client message
					this->receivedWindowCloseEvent();
				} else if (currentEvent.type == ConfigureNotify) {
					XConfigureEvent xce = currentEvent.xconfigure;
					if (this->windowWidth != xce.width || this->windowHeight != xce.height) {
						this->windowWidth = xce.width;
						this->windowHeight = xce.height;
						// Make a request to resize the canvas
						this->receivedWindowResize(xce.width, xce.height);
					}
				} else if (currentEvent.type == SelectionRequest) {
					// Based on: https://handmade.network/forums/articles/t/8544-implementing_copy_paste_in_x11
					// Another program has requested the content that you posted about in the clipboard.
					XSelectionRequestEvent request = currentEvent.xselectionrequest;	
					if (XGetSelectionOwner(this->display, this->clipboardAtom) == this->window && request.selection == this->clipboardAtom) {
						if (request.target == this->targetsAtom && request.property != None) {
							XChangeProperty(request.display, request.requestor, request.property,
								XA_ATOM, 32, PropModeReplace, (unsigned char*)&(this->utf8StringAtom), 1);
						} else if (request.target == this->utf8StringAtom && request.property != None) {
							// Encode the data as UTF-8 with portable line-breaks, without byte order mark nor null terminator.
							dsr::Buffer encodedUTF8 = dsr::string_saveToMemory(this->textToClipboard, dsr::CharacterEncoding::BOM_UTF8, dsr::LineEncoding::CrLf, false, false);
							XChangeProperty(request.display, request.requestor, request.property,
								request.target, 8, PropModeReplace, dsr::buffer_dangerous_getUnsafeData(encodedUTF8), dsr::buffer_getSize(encodedUTF8));
						}
						XSelectionEvent sendEvent;
						sendEvent.type = SelectionNotify;
						sendEvent.serial = request.serial;
						sendEvent.send_event = request.send_event;
						sendEvent.display = request.display;
						sendEvent.requestor = request.requestor;
						sendEvent.selection = request.selection;
						sendEvent.target = request.target;
						sendEvent.property = request.property;
						sendEvent.time = request.time;
						XSendEvent(display, request.requestor, 0, 0, (XEvent*)&sendEvent);
					}
				} else if (currentEvent.type == SelectionNotify) {
					// You previously requested access to a program's clipboard content and here it is giving the data to you.
					// Based on: https://handmade.network/forums/articles/t/8544-implementing_copy_paste_in_x11
					XSelectionEvent selection = currentEvent.xselection;
					if (selection.property == None) {
						// If we got an empty notification, we can avoid waiting for a timeout.
						this->loadingFromClipboard = false;
					} else {
						Atom actualType;
						int actualFormat;
						unsigned long bytesAfter;
						unsigned char* data;
						unsigned long count;
						XGetWindowProperty(this->display, this->window, this->clipboardAtom, 0, LONG_MAX, False, AnyPropertyType,
							&actualType, &actualFormat, &count, &bytesAfter, &data);
						if (selection.target == this->targetsAtom) {
							Atom* list = (Atom*)data;
							for (unsigned long i = 0; i < count; i++) {
								if (list[i] == XA_STRING) {
									this->targetAtom = XA_STRING;
								} else if (list[i] == this->utf8StringAtom) {
									this->targetAtom = this->utf8StringAtom;
									break;
								}
							}
							if (this->targetAtom != None) {
								XConvertSelection(this->display, this->clipboardAtom, this->targetAtom, this->clipboardAtom, this->window, CurrentTime);
							}
						} else if (selection.target == this->targetAtom) {
							dsr::Buffer textBuffer = dsr::buffer_create(count + 4); // Null terminate by adding zero initialized data after the copy.
							memcpy(buffer_dangerous_getUnsafeData(textBuffer), data, count);
							this->textFromClipboard = dsr::string_dangerous_decodeFromData(buffer_dangerous_getUnsafeData(textBuffer), dsr::CharacterEncoding::BOM_UTF8);
							// Stop waiting now that we found the data.
							this->loadingFromClipboard = false;
						}
						if (data) XFree(data);
					}
				}
			}
		}
		unlockWindow();
	}
}

static int destroyXImage(XImage *image) {
	if (image != nullptr) {
		if (image->data) {
			dsr::heap_decreaseUseCount(image->data);
		}
		image->data = nullptr;
		return XDestroyImage(image);
	}
	return 0;
}

// Locked because it overrides
void X11Window::resizeCanvas(int width, int height) {
	lockWindow();
		if (this->display) {
			unsigned int defaultDepth = DefaultDepth(this->display, XDefaultScreen(this->display));
			// Get the old canvas
			dsr::AlignedImageRgbaU8 oldCanvas = this->canvas[this->showIndex];
			for (int b = 0; b < bufferCount; b++) {
				// Create a new canvas
				this->canvas[b] = dsr::image_create_RgbaU8_native(width, height, this->packOrderIndex);
				// Copy from any old canvas
				if (dsr::image_exists(oldCanvas)) {
					dsr::draw_copy(this->canvas[b], oldCanvas);
				}
				// Get a pointer to the pixels
				uint8_t* rawData = dsr::image_dangerous_getData(this->canvas[b]);
				// Create an image in XLib using the pointer
				//   XLib takes ownership of the data
				this->canvasX[b] = XCreateImage(
				  this->display, CopyFromParent, defaultDepth, ZPixmap, 0, (char*)rawData,
				  dsr::image_getWidth(this->canvas[b]), dsr::image_getHeight(this->canvas[b]), 32, dsr::image_getStride(this->canvas[b])
				);
				// When the canvas image buffer is garbage collected, the destructor will call XLib to free the memory
				XImage *image = this->canvasX[b];
				// Tell the pixel buffer to also deallocate the XImage when the pixel data is about to be freed by the memory allocator.
				dsr::image_dangerous_replaceDestructor(this->canvas[b], dsr::HeapDestructor([](void *pixels, void *image) {
					XDestroyImage((XImage*)image);
				}, image));
				// Increase use count manually for the reference counted pixel buffer in XImage.
				dsr::heap_increaseUseCount(image->data);
				// And when closing the window externally, to trigger XDestroyImage, we should decrease reference count for the canvas.
				image->f.destroy_image = destroyXImage;
			}
		}
	unlockWindow();
}

X11Window::~X11Window() {
	#ifndef DISABLE_MULTI_THREADING
		// Wait for the last update of the window to finish so that it doesn't try to operate on freed resources
		if (this->displayFuture.valid()) {
			this->displayFuture.wait();
		}
	#endif
	lockWindow();
		if (this->display) {
			this->terminateClipboard();
			XFreeCursor(this->display, this->noCursor);
			XFreeGC(this->display, this->graphicsContext);
			XDestroyWindow(this->display, this->window);
			XCloseDisplay(this->display);
			this->display = nullptr;
		}
	unlockWindow();
}

void X11Window::showCanvas() {
	if (this->display) {
		#ifndef DISABLE_MULTI_THREADING
			// Wait for the previous update to finish, to avoid flooding the system with new threads waiting for windowLock
			if (this->displayFuture.valid()) {
				this->displayFuture.wait();
			}
		#endif
		this->drawIndex = (this->drawIndex + 1) % bufferCount;
		this->showIndex = (this->showIndex + 1) % bufferCount;
		this->prefetchEvents();
		int displayIndex = this->showIndex;
		lockWindow();
		std::function<void()> task = [this, displayIndex]() {
				// Clamp canvas dimensions to the target window
				int width = std::min(dsr::image_getWidth(this->canvas[displayIndex]), this->windowWidth);
				int height = std::min(dsr::image_getHeight(this->canvas[displayIndex]), this->windowHeight);
				// Display the result
				XPutImage(this->display, this->window, this->graphicsContext, this->canvasX[displayIndex], 0, 0, 0, 0, width, height);
			unlockWindow();
		};
		#ifdef DISABLE_MULTI_THREADING
			// Perform instantly
			task();
		#else
			if (this->firstFrame) {
				// The first frame will be cloned when double buffering.
				if (bufferCount == 2) {
					dsr::draw_copy(this->canvas[this->drawIndex], this->canvas[this->showIndex]);
				}
				// Single-thread the first frame to keep it safe.
				task();
				this->firstFrame = false;
			} else {
				// Run in the background while doing other things
				this->displayFuture = std::async(std::launch::async, task);
			}
		#endif
	}
}

dsr::Handle<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	if (XOpenDisplay(nullptr) != nullptr) {
		return dsr::handle_create<X11Window>(title, width, height);
	} else {
		dsr::sendWarning("Failed to create an X11 window.\n");
		return dsr::Handle<dsr::BackendWindow>();
	}
}
