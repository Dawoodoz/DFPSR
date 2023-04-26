
// Avoid cluttering the global namespace by hiding these from the header
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

#include "../DFPSR/api/imageAPI.h"
#include "../DFPSR/api/drawAPI.h"
#include "../DFPSR/gui/BackendWindow.h"

// According to this documentation, XInitThreads doesn't have to be used if a mutex is wrapped around all the calls to XLib.
//   https://tronche.com/gui/x/xlib/display/XInitThreads.html
#include <mutex>
#include <future>

static std::mutex windowLock;

// Enable this macro to disable multi-threading
//#define DISABLE_MULTI_THREADING

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
	void setCursorPosition(int x, int y) override;

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
	// Interface
	void setFullScreen(bool enabled) override;
	bool isFullScreen() override { return this->windowState == 2; }
	void showCanvas() override;
};

void X11Window::setCursorPosition(int x, int y) {
	windowLock.lock();
		XWarpPointer(this->display, this->window, this->window, 0, 0, this->windowWidth, this->windowHeight, x, y);
	windowLock.unlock();
}

void X11Window::applyCursorVisibility_locked() {
	windowLock.lock();
		if (this->visibleCursor) {
			// Reset to parent cursor
			XUndefineCursor(this->display, this->window);
		} else {
			// Let the window display an empty cursor
			XDefineCursor(this->display, this->window, this->noCursor);
		}
	windowLock.unlock();
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
	windowLock.lock();
		XSetStandardProperties(this->display, this->window, this->title.toStdString().c_str(), "Icon", None, NULL, 0, NULL);
	windowLock.unlock();
}

dsr::PackOrderIndex X11Window::getColorFormat_locked() {
	windowLock.lock();
		XVisualInfo visualRequest;
		visualRequest.screen = 0;
		visualRequest.depth = 32;
		visualRequest.c_class = TrueColor;
		int visualCount;
		XVisualInfo *formatList = XGetVisualInfo(this->display, VisualScreenMask | VisualDepthMask | VisualClassMask, &visualRequest, &visualCount);
		dsr::PackOrderIndex result = dsr::PackOrderIndex::RGBA;
		if (formatList == nullptr) {
			dsr::throwError(U"Error! The display does not support truecolor formats.\n");
		} else {
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
						dsr::throwError(U"Error! Unhandled color format. Only RGBA, ARGB, BGRA and ABGR are currently supported.\n");
					}
					break;
				}
			}
			XFree(formatList);
		}
	windowLock.unlock();
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
}

void X11Window::removeOldWindow_locked() {
	windowLock.lock();
		if (this->windowState != 0) {
			XFreeGC(this->display, this->graphicsContext);
			XDestroyWindow(this->display, this->window);
			XUngrabPointer(this->display, CurrentTime);
		}
		this->windowState = 0;
	windowLock.unlock();
}

void X11Window::prepareWindow_locked() {
	windowLock.lock();
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
	windowLock.unlock();
	// Reallocate the canvas
	this->resizeCanvas(this->windowWidth, this->windowHeight);
}

void X11Window::createGCWindow_locked(const dsr::String& title, int width, int height) {
	windowLock.lock();
		// Request to resize the canvas and interface according to the new window
		this->windowWidth = width;
		this->windowHeight = height;
		this->receivedWindowResize(width, height);
		int screenIndex = DefaultScreen(this->display);
		unsigned long black = BlackPixel(this->display, screenIndex);
		unsigned long white = WhitePixel(this->display, screenIndex);
		// Create a new window
		this->window = XCreateSimpleWindow(this->display, DefaultRootWindow(this->display), 0, 0, width, height, 0, white, black);
	windowLock.unlock();

	this->updateTitle_locked();

	windowLock.lock();
		// Create a new graphics context
		this->graphicsContext = XCreateGC(this->display, this->window, 0, 0);
		XSetBackground(this->display, this->graphicsContext, black);
		XSetForeground(this->display, this->graphicsContext, white);
		XClearWindow(this->display, this->window);
	windowLock.unlock();
}

void X11Window::createWindowed_locked(const dsr::String& title, int width, int height) {
	// Create the window
	this->createGCWindow_locked(title, width, height);
	windowLock.lock();
		// Display the window when done placing it
		XMapRaised(this->display, this->window);

		this->windowState = 1;
		this->firstFrame = true;
	windowLock.unlock();
	this->prepareWindow_locked();
}

void X11Window::createFullscreen_locked() {
	windowLock.lock();
		// Get the screen resolution
		Screen* screenInfo = DefaultScreenOfDisplay(this->display);
	windowLock.unlock();

	// Create the window
	this->createGCWindow_locked(U"", screenInfo->width, screenInfo->height);

	windowLock.lock();
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
	windowLock.unlock();
	this->prepareWindow_locked();
}

X11Window::X11Window(const dsr::String& title, int width, int height) {
	bool fullScreen = false;
	if (width < 1 || height < 1) {
		fullScreen = true;
		width = 400;
		height = 300;
	}

	windowLock.lock();
		this->display = XOpenDisplay(nullptr);
	windowLock.unlock();
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
	} else if (keyCode == XK_Shift_L) {
		result = dsr::DsrKey_LeftShift;
	} else if (keyCode == XK_Shift_R) {
		result = dsr::DsrKey_RightShift;
	} else if (keyCode == XK_Control_L) {
		result = dsr::DsrKey_LeftControl;
	} else if (keyCode == XK_Control_R) {
		result = dsr::DsrKey_RightControl;
	} else if (keyCode == XK_Alt_L) {
		result = dsr::DsrKey_LeftAlt;
	} else if (keyCode == XK_Alt_R) {
		result = dsr::DsrKey_RightAlt;
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
					this->queueInputEvent(new dsr::WindowEvent(dsr::WindowEventType::Redraw, this->windowWidth, this->windowHeight));
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
						this->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyType, character, dsrKey));
						// Skip next event
						XNextEvent(this->display, &currentEvent);
					} else {
						if (currentEvent.type == KeyPress) {
							// Physical key down
							this->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyDown, character, dsrKey));
							// First press typing
							this->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyType, character, dsrKey));
						} else { // currentEvent.type == KeyRelease
							// Physical key up
							this->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyUp, character, dsrKey));
						}
					}
				} else if (currentEvent.type == ButtonPress || currentEvent.type == ButtonRelease) {
					dsr::MouseKeyEnum key = getMouseKey(currentEvent.xbutton.button);
					if (isVerticalScrollKey(key)) {
						// Scroll down/up
						if (!hasScrolled) {
							this->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::Scroll, key, dsr::IVector2D(currentEvent.xbutton.x, currentEvent.xbutton.y)));
						}
						hasScrolled = true;
					} else {
						// Mouse down/up
						this->queueInputEvent(new dsr::MouseEvent(currentEvent.type == ButtonPress ? dsr::MouseEventType::MouseDown : dsr::MouseEventType::MouseUp, key, dsr::IVector2D(currentEvent.xbutton.x, currentEvent.xbutton.y)));
					}
				} else if (currentEvent.type == MotionNotify) {
					// Mouse move
					this->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseMove, dsr::MouseKeyEnum::NoKey, dsr::IVector2D(currentEvent.xmotion.x, currentEvent.xmotion.y)));
				} else if (currentEvent.type == ClientMessage) {
					// Close
					//   Assume WM_DELETE_WINDOW since it is the only registered client message
					this->queueInputEvent(new dsr::WindowEvent(dsr::WindowEventType::Close, this->windowWidth, this->windowHeight));
				} else if (currentEvent.type == ConfigureNotify) {
					XConfigureEvent xce = currentEvent.xconfigure;
					if (this->windowWidth != xce.width || this->windowHeight != xce.height) {
						this->windowWidth = xce.width;
						this->windowHeight = xce.height;
						// Make a request to resize the canvas
						this->receivedWindowResize(xce.width, xce.height);
					}
				}
			}
		}
		windowLock.unlock();
	}
}

// Locked because it overrides
void X11Window::resizeCanvas(int width, int height) {
	windowLock.lock();
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
				dsr::image_dangerous_replaceDestructor(this->canvas[b], [image](uint8_t *data) { XDestroyImage(image); });
			}
		}
	windowLock.unlock();
}

X11Window::~X11Window() {
	#ifndef DISABLE_MULTI_THREADING
		// Wait for the last update of the window to finish so that it doesn't try to operate on freed resources
		if (this->displayFuture.valid()) {
			this->displayFuture.wait();
		}
	#endif
	windowLock.lock();
		if (this->display) {
			XFreeCursor(this->display, this->noCursor);
			XFreeGC(this->display, this->graphicsContext);
			XDestroyWindow(this->display, this->window);
			XCloseDisplay(this->display);
			this->display = nullptr;
		}
	windowLock.unlock();
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
		windowLock.lock();
		std::function<void()> task = [this, displayIndex]() {
				// Clamp canvas dimensions to the target window
				int width = std::min(dsr::image_getWidth(this->canvas[displayIndex]), this->windowWidth);
				int height = std::min(dsr::image_getHeight(this->canvas[displayIndex]), this->windowHeight);
				// Display the result
				XPutImage(this->display, this->window, this->graphicsContext, this->canvasX[displayIndex], 0, 0, 0, 0, width, height);
			windowLock.unlock();
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

std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	// Check if a display is available for creating a window
	if (XOpenDisplay(nullptr) != nullptr) {
		auto backend = std::make_shared<X11Window>(title, width, height);
		return std::dynamic_pointer_cast<dsr::BackendWindow>(backend);
	} else {
		dsr::printText("No display detected. Aborting X11 window creation.\n");
		return std::shared_ptr<dsr::BackendWindow>();
	}
}
