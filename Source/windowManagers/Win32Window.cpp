
/*
Link to these dependencies for MS Windows:
	gdi32
	user32
	kernel32
	comctl32
*/

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>

#include "../DFPSR/api/imageAPI.h"
#include "../DFPSR/api/drawAPI.h"
#include "../DFPSR/api/bufferAPI.h"
#include "../DFPSR/api/timeAPI.h"
#include "../DFPSR/gui/BackendWindow.h"

#include <mutex>
#include <future>

static std::mutex windowLock;

// Enable this macro to disable multi-threading
//#define DISABLE_MULTI_THREADING

static const int bufferCount = 2;

class Win32Window : public dsr::BackendWindow {
public:
	// The native windows handle
	HWND hwnd;
	// The cursors
	HCURSOR noCursor, defaultCursor;
	// Because scroll events don't give a cursor location, remember it from other mouse events.
	dsr::IVector2D lastMousePos;
	// Keep track of when the cursor is inside of the window,
	// so that we can show it again when leaving the window
	bool cursorIsInside = false;

	// Double buffering to allow drawing to a canvas while displaying the previous one
	dsr::AlignedImageRgbaU8 canvas[bufferCount];
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
private:
	// Called before the application fetches events from the input queue
	//   Closing the window, moving the mouse, pressing a key, et cetera
	void prefetchEvents() override;
	void prefetchEvents_impl();

	// Called to change the cursor visibility and returning true on success
	bool setCursorVisibility(bool visible) override;

	// Place the cursor within the window
	void setCursorPosition(int x, int y) override;
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
	void createWindowed_locked(const dsr::String& title, int width, int height);
	void createFullscreen_locked();
	void prepareWindow_locked();
	int windowState = 0; // 0=none, 1=windowed, 2=fullscreen
public:
	// Constructors
	Win32Window(const Win32Window&) = delete; // Non-copyable because of pointer aliasing.
	Win32Window(const dsr::String& title, int width, int height);
	int getWidth() const override { return this->windowWidth; };
	int getHeight() const override { return this->windowHeight; };
	// Destructor
	~Win32Window();
	// Interface
	void setFullScreen(bool enabled) override;
	bool isFullScreen() override { return this->windowState == 2; }
	void redraw(HWND& hwnd, bool locked, bool swap); // HWND is passed by argument because drawing might be called before the constructor has assigned it to this->hwnd
	void showCanvas() override;
	// Clipboard		
	dsr::ReadableString loadFromClipboard(double timeoutInSeconds);
	void saveToClipboard(const dsr::ReadableString &text, double timeoutInSeconds);
};

static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static TCHAR windowClassName[] = _T("DfpsrWindowApplication");

dsr::ReadableString Win32Window::loadFromClipboard(double timeoutInSeconds) {
	dsr::String result = U"";
	if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
		// TODO: Repeat attempts to open the clipboard in a delayed loop until the timeout is reached.
		if (OpenClipboard(this->hwnd)) {
			HGLOBAL globalBuffer = GetClipboardData(CF_UNICODETEXT);
			void *globalData = GlobalLock(globalBuffer);
			if (globalData) {
				result = dsr::string_dangerous_decodeFromData(globalData, dsr::CharacterEncoding::BOM_UTF16LE);
				GlobalUnlock(globalBuffer);
			}
			CloseClipboard();
		}
	} else if (IsClipboardFormatAvailable(CF_TEXT)) {
		// TODO: Repeat attempts to open the clipboard in a delayed loop until the timeout is reached.
		if (OpenClipboard(this->hwnd)) {
			HGLOBAL globalBuffer = GetClipboardData(CF_TEXT);
			void *globalData = GlobalLock(globalBuffer);
			if (globalData) {
				// TODO: Use a built-in conversion from native text formats.
				// If the text is not in Unicode format, assume Latin-1.
				result = dsr::string_dangerous_decodeFromData(globalData, dsr::CharacterEncoding::Raw_Latin1);
				GlobalUnlock(globalBuffer);
			}
			CloseClipboard();
		}
	}
	return result;
}

void Win32Window::saveToClipboard(const dsr::ReadableString &text, double timeoutInSeconds) {
	// TODO: Repeat attempts to open the clipboard in a delayed loop until the timeout is reached.
	if (OpenClipboard(this->hwnd)) {
		EmptyClipboard();
		dsr::Buffer savedText = dsr::string_saveToMemory(text, dsr::CharacterEncoding::BOM_UTF16LE, dsr::LineEncoding::CrLf, false, true);
		int64_t textSize = dsr::buffer_getSize(savedText);
		HGLOBAL globalBuffer = GlobalAlloc(GMEM_MOVEABLE, textSize);
		if (globalBuffer) {
			void *globalData = GlobalLock(globalBuffer);
			uint8_t *localData = dsr::buffer_dangerous_getUnsafeData(savedText);
			memcpy(globalData, localData, textSize);
			GlobalUnlock(globalBuffer);
			SetClipboardData(CF_UNICODETEXT, globalBuffer);
		} else {
			dsr::sendWarning(U"Could not allocate global memory for saving text to the clipboard!\n");
		}
		CloseClipboard();
	}
}

void Win32Window::updateTitle_locked() {
	windowLock.lock();
		if (!SetWindowTextA(this->hwnd, this->title.toStdString().c_str())) {
			dsr::printText("Warning! Could not assign the window title ", dsr::string_mangleQuote(this->title), ".\n");
		}
	windowLock.unlock();
}

// The method can be seen as locked, but it overrides a virtual method that is independent of threading.
void Win32Window::setCursorPosition(int x, int y) {
	windowLock.lock();
		POINT point; point.x = x; point.y = y;
		ClientToScreen(this->hwnd, &point);
		SetCursorPos(point.x, point.y);
	windowLock.unlock();
}

bool Win32Window::setCursorVisibility(bool visible) {
	// Cursor visibility is deferred, so no need to lock access here.
	// Remember the cursor's visibility for anyone asking
	this->visibleCursor = visible;
	// Indicate that the feature is implemented
	return true;
}

void Win32Window::setFullScreen(bool enabled) {
	if (this->windowState == 1 && enabled) {
		// Clean up any previous window
		removeOldWindow_locked();
		// Create the new window and graphics context
		this->createFullscreen_locked();
	} else if (this->windowState == 2 && !enabled) {
		// Clean up any previous window
		removeOldWindow_locked();
		// Create the new window and graphics context
		this->createWindowed_locked(this->title, 800, 600); // TODO: Remember the dimensions from last windowed mode
	}
}

void Win32Window::removeOldWindow_locked() {
	windowLock.lock();
		if (this->windowState != 0) {
			DestroyWindow(this->hwnd);
		}
	this->windowState = 0;
	windowLock.unlock();
}

void Win32Window::prepareWindow_locked() {
	windowLock.lock();
		// Reallocate the canvas
		this->resizeCanvas(this->windowWidth, this->windowHeight);
		// Show the window
		ShowWindow(this->hwnd, SW_NORMAL);
		// Repaint
		UpdateWindow(this->hwnd);
	windowLock.unlock();
}

static bool registered = false;
static void registerIfNeeded() {
	if (!registered) {
		// The Window structure
		WNDCLASSEX wincl;
		memset(&wincl, 0, sizeof(WNDCLASSEX));
		wincl.hInstance = NULL;
		wincl.lpszClassName = windowClassName;
		wincl.lpfnWndProc = WindowProcedure;
		wincl.style = 0;
		wincl.cbSize = sizeof(WNDCLASSEX);

		// Use default icon and mouse-pointer
		wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
		wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
		wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
		wincl.lpszMenuName = NULL;
		wincl.cbClsExtra = 0;
		wincl.cbWndExtra = sizeof(LPVOID);
		// Use Windows's default color as the background of the window
		wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND; // TODO: Make black

		// Register the window class, and if it fails quit the program
		if (!RegisterClassEx (&wincl)) {
			dsr::throwError("Call to RegisterClassEx failed!\n");
		}

		registered = true;
	}
}

void Win32Window::createWindowed_locked(const dsr::String& title, int width, int height) {
	// Request to resize the canvas and interface according to the new window
	this->windowWidth = width;
	this->windowHeight = height;
	this->receivedWindowResize(width, height);

	windowLock.lock();
		// Register the Window class during first creation
		registerIfNeeded();

		// The class is registered, let's create the program
		this->hwnd = CreateWindowEx(
		  0,                   // dwExStyle
		  windowClassName,     // lpClassName
		  _T(""),              // lpWindowName
		  WS_OVERLAPPEDWINDOW, // dwStyle
		  CW_USEDEFAULT,       // x
		  CW_USEDEFAULT,       // y
		  width,               // nWidth
		  height,              // nHeight
		  HWND_DESKTOP,        // hWndParent
		  NULL,	               // hMenu
		  NULL,                // hInstance
		  (LPVOID)this         // lpParam
		);
	windowLock.unlock();

	this->updateTitle_locked();

	this->windowState = 1;
	this->prepareWindow_locked();
}

void Win32Window::createFullscreen_locked() {
	windowLock.lock();
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		// Request to resize the canvas and interface according to the new window
		this->windowWidth = screenWidth;
		this->windowHeight = screenHeight;
		this->receivedWindowResize(screenWidth, screenHeight);

		// Register the Window class during first creation
		registerIfNeeded();

		// The class is registered, let's create the program
		this->hwnd = CreateWindowEx(
		  0,                     // dwExStyle
		  windowClassName,       // lpClassName
		  _T(""),                // lpWindowName
		  WS_POPUP | WS_VISIBLE, // dwStyle
		  0,                     // x
		  0,                     // y
		  screenWidth,           // nWidth
		  screenHeight,          // nHeight
		  HWND_DESKTOP,          // hWndParent
		  NULL,	                 // hMenu
		  NULL,                  // hInstance
		  (LPVOID)this           // lpParam
		);
	windowLock.unlock();

	this->windowState = 2;
	this->prepareWindow_locked();
}

Win32Window::Win32Window(const dsr::String& title, int width, int height) {
	bool fullScreen = false;
	if (width < 1 || height < 1) {
		fullScreen = true;
	}

	// Remember the title
	this->title = title;

	windowLock.lock();
		// Get the default cursor
		this->defaultCursor = LoadCursor(0, IDC_ARROW);

		// Create an invisible cursor using masks padded to 32 bits for safety
		uint32_t cursorAndMask = 0b11111111;
		uint32_t cursorXorMask = 0b00000000;
		this->noCursor = CreateCursor(NULL, 0, 0, 1, 1, (const void*)&cursorAndMask, (const void*)&cursorXorMask);
	windowLock.unlock();

	// Create a window
	if (fullScreen) {
		this->createFullscreen_locked();
	} else {
		this->createWindowed_locked(title, width, height);
	}
}

static dsr::DsrKey getDsrKey(WPARAM keyCode) {
	dsr::DsrKey result = dsr::DsrKey_Unhandled;
	if (keyCode == VK_ESCAPE) {
		result = dsr::DsrKey_Escape;
	} else if (keyCode == VK_F1) {
		result = dsr::DsrKey_F1;
	} else if (keyCode == VK_F2) {
		result = dsr::DsrKey_F2;
	} else if (keyCode == VK_F3) {
		result = dsr::DsrKey_F3;
	} else if (keyCode == VK_F4) {
		result = dsr::DsrKey_F4;
	} else if (keyCode == VK_F5) {
		result = dsr::DsrKey_F5;
	} else if (keyCode == VK_F6) {
		result = dsr::DsrKey_F6;
	} else if (keyCode == VK_F7) {
		result = dsr::DsrKey_F7;
	} else if (keyCode == VK_F8) {
		result = dsr::DsrKey_F8;
	} else if (keyCode == VK_F9) {
		result = dsr::DsrKey_F9;
	} else if (keyCode == VK_F10) {
		result = dsr::DsrKey_F10;
	} else if (keyCode == VK_F11) {
		result = dsr::DsrKey_F11;
	} else if (keyCode == VK_F12) {
		result = dsr::DsrKey_F12;
	} else if (keyCode == VK_PAUSE) {
		result = dsr::DsrKey_Pause;
	} else if (keyCode == VK_SPACE) {
		result = dsr::DsrKey_Space;
	} else if (keyCode == VK_TAB) {
		result = dsr::DsrKey_Tab;
	} else if (keyCode == VK_RETURN) {
		result = dsr::DsrKey_Return;
	} else if (keyCode == VK_BACK) {
		result = dsr::DsrKey_BackSpace;
	} else if (keyCode == VK_LSHIFT || keyCode == VK_SHIFT || keyCode == VK_RSHIFT) {
		result = dsr::DsrKey_Shift;
	} else if (keyCode == VK_LCONTROL || keyCode == VK_CONTROL || keyCode == VK_RCONTROL) {
		result = dsr::DsrKey_Control;
	} else if (keyCode == VK_LMENU || keyCode == VK_MENU || keyCode == VK_RMENU) {
		result = dsr::DsrKey_Alt;
	} else if (keyCode == VK_DELETE) {
		result = dsr::DsrKey_Delete;
	} else if (keyCode == VK_LEFT) {
		result = dsr::DsrKey_LeftArrow;
	} else if (keyCode == VK_RIGHT) {
		result = dsr::DsrKey_RightArrow;
	} else if (keyCode == VK_UP) {
		result = dsr::DsrKey_UpArrow;
	} else if (keyCode == VK_DOWN) {
		result = dsr::DsrKey_DownArrow;
	} else if (keyCode == 0x30) {
		result = dsr::DsrKey_0;
	} else if (keyCode == 0x31) {
		result = dsr::DsrKey_1;
	} else if (keyCode == 0x32) {
		result = dsr::DsrKey_2;
	} else if (keyCode == 0x33) {
		result = dsr::DsrKey_3;
	} else if (keyCode == 0x34) {
		result = dsr::DsrKey_4;
	} else if (keyCode == 0x35) {
		result = dsr::DsrKey_5;
	} else if (keyCode == 0x36) {
		result = dsr::DsrKey_6;
	} else if (keyCode == 0x37) {
		result = dsr::DsrKey_7;
	} else if (keyCode == 0x38) {
		result = dsr::DsrKey_8;
	} else if (keyCode == 0x39) {
		result = dsr::DsrKey_9;
	} else if (keyCode == 0x41) {
		result = dsr::DsrKey_A;
	} else if (keyCode == 0x42) {
		result = dsr::DsrKey_B;
	} else if (keyCode == 0x43) {
		result = dsr::DsrKey_C;
	} else if (keyCode == 0x44) {
		result = dsr::DsrKey_D;
	} else if (keyCode == 0x45) {
		result = dsr::DsrKey_E;
	} else if (keyCode == 0x46) {
		result = dsr::DsrKey_F;
	} else if (keyCode == 0x47) {
		result = dsr::DsrKey_G;
	} else if (keyCode == 0x48) {
		result = dsr::DsrKey_H;
	} else if (keyCode == 0x49) {
		result = dsr::DsrKey_I;
	} else if (keyCode == 0x4A) {
		result = dsr::DsrKey_J;
	} else if (keyCode == 0x4B) {
		result = dsr::DsrKey_K;
	} else if (keyCode == 0x4C) {
		result = dsr::DsrKey_L;
	} else if (keyCode == 0x4D) {
		result = dsr::DsrKey_M;
	} else if (keyCode == 0x4E) {
		result = dsr::DsrKey_N;
	} else if (keyCode == 0x4F) {
		result = dsr::DsrKey_O;
	} else if (keyCode == 0x50) {
		result = dsr::DsrKey_P;
	} else if (keyCode == 0x51) {
		result = dsr::DsrKey_Q;
	} else if (keyCode == 0x52) {
		result = dsr::DsrKey_R;
	} else if (keyCode == 0x53) {
		result = dsr::DsrKey_S;
	} else if (keyCode == 0x54) {
		result = dsr::DsrKey_T;
	} else if (keyCode == 0x55) {
		result = dsr::DsrKey_U;
	} else if (keyCode == 0x56) {
		result = dsr::DsrKey_V;
	} else if (keyCode == 0x57) {
		result = dsr::DsrKey_W;
	} else if (keyCode == 0x58) {
		result = dsr::DsrKey_X;
	} else if (keyCode == 0x59) {
		result = dsr::DsrKey_Y;
	} else if (keyCode == 0x5A) {
		result = dsr::DsrKey_Z;
	} else if (keyCode == 0x2D) {
		result = dsr::DsrKey_Insert;
	} else if (keyCode == 0x24) {
		result = dsr::DsrKey_Home;
	} else if (keyCode == 0x23) {
		result = dsr::DsrKey_End;
	} else if (keyCode == 0x21) {
		result = dsr::DsrKey_PageUp;
	} else if (keyCode == 0x22) {
		result = dsr::DsrKey_PageDown;
	}
	return result;
}

// Called from DispatchMessage via prefetchEvents
static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// Get the Win32Window owning the given hwnd
	Win32Window *parent = nullptr;
	if (message == WM_CREATE) {
		// Cast the pointer argument into CREATESTRUCT and get the lParam given to the window on creation
		CREATESTRUCT *createStruct = (CREATESTRUCT*)lParam;
		parent = (Win32Window*)createStruct->lpCreateParams;
		if (parent == nullptr) {
			dsr::throwError("Null handle retreived from lParam (", (intptr_t)parent, ") in WM_CREATE message.\n");
		}
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (intptr_t)parent);
	} else {
		// Get the parent
		parent = (Win32Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		if (parent == nullptr) {
			// Don't try to handle global events unrelated to any window
			return DefWindowProc(hwnd, message, wParam, lParam);
		}
	}
	// Get the cursor location relative to the window.
	// Excluding scroll events that don't provide valid cursor locations.
	if (message == WM_LBUTTONDOWN
	 || message == WM_LBUTTONUP
	 || message == WM_RBUTTONDOWN
	 || message == WM_RBUTTONUP
	 || message == WM_MBUTTONDOWN
	 || message == WM_MBUTTONUP
	 || message == WM_MOUSEMOVE) {
		parent->lastMousePos = dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	}
	// Check that we're using the correct window instance (This might not be the case while toggling full-screen)
	// Handle the message
	int result = 0;
	switch (message) {
	case WM_QUIT:
		PostQuitMessage(wParam);
		break;
	case WM_CLOSE:
		parent->queueInputEvent(new dsr::WindowEvent(dsr::WindowEventType::Close, parent->windowWidth, parent->windowHeight));
		DestroyWindow(hwnd);
		break;
	case WM_LBUTTONDOWN:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseDown, dsr::MouseKeyEnum::Left, parent->lastMousePos));
		break;
	case WM_LBUTTONUP:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseUp, dsr::MouseKeyEnum::Left, parent->lastMousePos));
		break;
	case WM_RBUTTONDOWN:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseDown, dsr::MouseKeyEnum::Right, parent->lastMousePos));
		break;
	case WM_RBUTTONUP:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseUp, dsr::MouseKeyEnum::Right, parent->lastMousePos));
		break;
	case WM_MBUTTONDOWN:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseDown, dsr::MouseKeyEnum::Middle, parent->lastMousePos));
		break;
	case WM_MBUTTONUP:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseUp, dsr::MouseKeyEnum::Middle, parent->lastMousePos));
		break;
	case WM_MOUSEMOVE:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseMove, dsr::MouseKeyEnum::NoKey, parent->lastMousePos));
		break;
	case WM_SETCURSOR:
		if (LOWORD(lParam) == HTCLIENT) {
			if (parent->visibleCursor) {
				SetCursor(parent->defaultCursor);
			} else {
				SetCursor(parent->noCursor);
			}
		}
		break;
	case WM_MOUSEWHEEL:
		{
			int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (delta > 0) {
				parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::Scroll, dsr::MouseKeyEnum::ScrollUp, parent->lastMousePos));
			} else if (delta < 0) {
				parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::Scroll, dsr::MouseKeyEnum::ScrollDown, parent->lastMousePos));
			}
		}
		break;
	case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_KEYUP: case WM_SYSKEYUP:
		{
			dsr::DsrChar character;
			if (IsWindowUnicode(hwnd)) {
				dsr::CharacterEncoding encoding = dsr::CharacterEncoding::BOM_UTF16LE;
				dsr::String characterString = dsr::string_dangerous_decodeFromData((const void*)&wParam, encoding);
				character = characterString[0]; // Convert from UTF-16 surrogate to UTF-32 Unicode character
			} else {
				character = wParam; // Raw ansi character
			}
			dsr::DsrKey dsrKey = getDsrKey(wParam); // Portable key-code
			
			bool previouslyPressed = lParam & (1 << 30);
			// For now, just let Windows send both Alt and Ctrl events from AltGr.
			//   Would however be better if it could be consistent with other platforms.
			if (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) {
				// If not repeated
				if (!previouslyPressed) {
					// Physical key down
					parent->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyDown, character, dsrKey));
				}
				// Press typing with repeat
				parent->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyType, character, dsrKey));
			} else { // message == WM_KEYUP || message == WM_SYSKEYUP
				// Physical key up
				parent->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyUp, character, dsrKey));
			}
		}
		break;
	case WM_PAINT:
		//parent->queueInputEvent(new dsr::WindowEvent(dsr::WindowEventType::Redraw, parent->windowWidth, parent->windowHeight));
		// BeginPaint and EndPaint must be called with the given hwnd to prevent having the redraw message sent again
		parent->redraw(hwnd, false, false);
		// Passing on the event to prevent flooding with more messages. This is only a temporary solution.
		// TODO: Avoid overwriting the result with any background color.
		//result = DefWindowProc(hwnd, message, wParam, lParam);
		break;
	case WM_SIZE:
		// If there's no size during minimization, don't try to resize the canvas
		if (wParam != SIZE_MINIMIZED) {
			int width = LOWORD(lParam);
			int height = HIWORD(lParam);
			parent->windowWidth = width;
			parent->windowHeight = height;
			parent->receivedWindowResize(width, height);
		}
		// Resize the window as requested
		result = DefWindowProc(hwnd, message, wParam, lParam);
		break;
	default:
		result = DefWindowProc(hwnd, message, wParam, lParam);
	}
	return result;
}

void Win32Window::prefetchEvents_impl() {
	MSG messages;
	if (IsWindowUnicode(this->hwnd)) {
		while (PeekMessageW(&messages, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&messages); DispatchMessage(&messages); }
	} else {
		while (PeekMessage(&messages, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&messages); DispatchMessage(&messages); }
	}
}

void Win32Window::prefetchEvents() {
	// Only prefetch new events if nothing else is locking.
	if (windowLock.try_lock()) {
		this->prefetchEvents_impl();
		windowLock.unlock();
	}
}

// Locked because it overrides
void Win32Window::resizeCanvas(int width, int height) {
	// Create a new canvas
	//   Even thou Windows is using RGBA pack order for the window, the bitmap format used for drawing is using BGRA order
	for (int bufferIndex = 0; bufferIndex < bufferCount; bufferIndex++) {
		dsr::AlignedImageRgbaU8 previousCanvas = this->canvas[bufferIndex];
		this->canvas[bufferIndex] = dsr::image_create_RgbaU8_native(width, height, dsr::PackOrderIndex::BGRA);
		if (image_exists(previousCanvas)) {
			// Until the application's main loop has redrawn, fill the new canvas with a copy of the old one with black borders.
			dsr::draw_copy(this->canvas[bufferIndex], previousCanvas);
		}
	}
	this->firstFrame = true;
}

Win32Window::~Win32Window() {
	#ifndef DISABLE_MULTI_THREADING
		// Wait for the last update of the window to finish so that it doesn't try to operate on freed resources
		if (this->displayFuture.valid()) {
			this->displayFuture.wait();
		}
	#endif
	windowLock.lock();
		// Destroy the invisible cursor
		DestroyCursor(this->noCursor);
		// Destroy the native window
		DestroyWindow(this->hwnd);
	windowLock.unlock();
}

// The lock argument must be true if not already within a lock and false if inside of a lock.
void Win32Window::redraw(HWND& hwnd, bool lock, bool swap) {
	#ifndef DISABLE_MULTI_THREADING
		// Wait for the previous update to finish, to avoid flooding the system with new threads waiting for windowLock
		if (this->displayFuture.valid()) {
			this->displayFuture.wait();
		}
	#endif

	if (lock) {
		// Any other requests will have to wait.
		windowLock.lock();
		// Last chance to prefetch events before uploading the canvas.
		this->prefetchEvents_impl();
	}
	if (swap) {
		this->drawIndex = (this->drawIndex + 1) % bufferCount;
		this->showIndex = (this->showIndex + 1) % bufferCount;
	}
	this->prefetchEvents();
	int displayIndex = this->showIndex;
	std::function<void()> task = [this, displayIndex, lock]() {
		// Let the source bitmap use a padded width to safely handle the stride
		// Windows requires 8-byte alignment, but the image format uses larger alignment.
		int paddedWidth = dsr::image_getStride(this->canvas[displayIndex]) / 4;
		//int width = dsr::image_getWidth(this->canvas[displayIndex]);
		int height = dsr::image_getHeight(this->canvas[displayIndex]);
		InvalidateRect(this->hwnd, NULL, false);
		PAINTSTRUCT paintStruct;
		HDC targetContext = BeginPaint(this->hwnd, &paintStruct);
			BITMAPINFO bmi = {};
			bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
			bmi.bmiHeader.biWidth = paddedWidth;
			bmi.bmiHeader.biHeight = -height;
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 32;
			bmi.bmiHeader.biCompression = BI_RGB;
			SetDIBitsToDevice(targetContext, 0, 0, paddedWidth, height, 0, 0, 0, height, dsr::image_dangerous_getData(this->canvas[displayIndex]), &bmi, DIB_RGB_COLORS);
		EndPaint(this->hwnd, &paintStruct);
		if (lock) {
			windowLock.unlock();
		}
	};
	#ifdef DISABLE_MULTI_THREADING
		// Perform instantly
		task();
	#else
		if (this->firstFrame) {
			// The first frame will be cloned when double buffering.
			if (bufferCount == 2) {
				// TODO: Only do this for the absolute first frame, not after resize.
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

void Win32Window::showCanvas() {
	this->redraw(this->hwnd, true, true);
}

std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	auto backend = std::make_shared<Win32Window>(title, width, height);
	return std::dynamic_pointer_cast<dsr::BackendWindow>(backend);
}
