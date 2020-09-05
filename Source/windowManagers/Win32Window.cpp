
#include "../DFPSR/api/imageAPI.h"
#include "../DFPSR/gui/BackendWindow.h"

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

class Win32Window : public dsr::BackendWindow {
public:
	// The native windows handle
	HWND hwnd;
	// Double buffering to allow drawing to a canvas while displaying the previous one
	// The image which can be drawn to
	dsr::AlignedImageRgbaU8 canvas;
	// Remembers the dimensions of the window from creation and resize events
	//   This allow requesting the size of the window at any time
	int windowWidth = 0, windowHeight = 0;
private:
	// Called before the application fetches events from the input queue
	//   Closing the window, moving the mouse, pressing a key, et cetera
	void prefetchEvents() override;
private:
	// Helper methods specific to calling XLib
	void updateTitle();
private:
	// Canvas methods
	dsr::AlignedImageRgbaU8 getCanvas() override { return this->canvas; }
	void resizeCanvas(int width, int height) override;
	// Window methods
	void setTitle(const dsr::String &newTitle) override {
		this->title = newTitle;
		this->updateTitle();
	}
	void removeOldWindow();
	void createWindowed(const dsr::String& title, int width, int height);
	void createFullscreen();
	void prepareWindow();
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
	void redraw(HWND& hwnd); // HWND is passed by argument because drawing might be called before the constructor has assigned it to this->hwnd
	void showCanvas() override;
};

static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

static TCHAR windowClassName[] = _T("DfpsrWindowApplication");

void Win32Window::updateTitle() {
	if (!SetWindowTextA(this->hwnd, this->title.toStdString().c_str())) {
		dsr::printText("Warning! Could not assign the window title ", dsr::string_mangleQuote(this->title), ".\n");
	}
}

void Win32Window::setFullScreen(bool enabled) {
	if (this->windowState == 1 && enabled) {
		// Clean up any previous window
		removeOldWindow();
		// Create the new window and graphics context
		this->createFullscreen();
	} else if (this->windowState == 2 && !enabled) {
		// Clean up any previous window
		removeOldWindow();
		// Create the new window and graphics context
		this->createWindowed(this->title, 800, 600); // TODO: Remember the dimensions from last windowed mode
	}
}

void Win32Window::removeOldWindow() {
	if (this->windowState != 0) {

		DestroyWindow(this->hwnd);
	}
	this->windowState = 0;
}

void Win32Window::prepareWindow() {
	// Reallocate the canvas
	this->resizeCanvas(this->windowWidth, this->windowHeight);
	// Show the window
	ShowWindow(this->hwnd, SW_NORMAL);
	// Repaint
	UpdateWindow(this->hwnd);
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

void Win32Window::createWindowed(const dsr::String& title, int width, int height) {
	// Request to resize the canvas and interface according to the new window
	this->windowWidth = width;
	this->windowHeight = height;
	this->receivedWindowResize(width, height);

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

	this->updateTitle();

	this->windowState = 1;
	this->prepareWindow();
}

void Win32Window::createFullscreen() {
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

	this->windowState = 2;
	this->prepareWindow();
}

Win32Window::Win32Window(const dsr::String& title, int width, int height) {
	bool fullScreen = false;
	if (width < 1 || height < 1) {
		fullScreen = true;
	}

	// Remember the title
	this->title = title;

	// Create a window
	if (fullScreen) {
		this->createFullscreen();
	} else {
		this->createWindowed(title, width, height);
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
	} else if (keyCode == VK_LSHIFT) {
		result = dsr::DsrKey_LeftShift;
	} else if (keyCode == VK_RSHIFT) {
		result = dsr::DsrKey_RightShift;
	} else if (keyCode == VK_LCONTROL) {
		result = dsr::DsrKey_LeftControl;
	} else if (keyCode == VK_RCONTROL) {
		result = dsr::DsrKey_RightControl;
	} else if (keyCode == VK_LMENU) {
		result = dsr::DsrKey_LeftAlt;
	} else if (keyCode == VK_RMENU) {
		result = dsr::DsrKey_RightAlt;
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
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseDown, dsr::MouseKeyEnum::Left, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
		break;
	case WM_LBUTTONUP:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseUp, dsr::MouseKeyEnum::Left, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
		break;
	case WM_RBUTTONDOWN:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseDown, dsr::MouseKeyEnum::Right, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
		break;
	case WM_RBUTTONUP:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseUp, dsr::MouseKeyEnum::Right, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
		break;
	case WM_MBUTTONDOWN:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseDown, dsr::MouseKeyEnum::Middle, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
		break;
	case WM_MBUTTONUP:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseUp, dsr::MouseKeyEnum::Middle, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
		break;
	case WM_MOUSEMOVE:
		parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::MouseMove, dsr::MouseKeyEnum::NoKey, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
		break;
	case WM_MOUSEWHEEL:
		{
			int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			if (delta > 0) {
				parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::Scroll, dsr::MouseKeyEnum::ScrollUp, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
			} else if (delta < 0) {
				parent->queueInputEvent(new dsr::MouseEvent(dsr::MouseEventType::Scroll, dsr::MouseKeyEnum::ScrollDown, dsr::IVector2D(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam))));
			}
		}
		break;
	case WM_KEYDOWN: case WM_KEYUP:
		{
			char character = wParam; // System specific key-code
			dsr::DsrKey dsrKey = getDsrKey(wParam); // Portable key-code
			if (message == WM_KEYDOWN) {
				// Physical key down
				parent->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyDown, character, dsrKey));
				// First press typing
				parent->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyType, character, dsrKey));
			} else { // message == WM_KEYUP
				// Physical key up
				parent->queueInputEvent(new dsr::KeyboardEvent(dsr::KeyboardEventType::KeyUp, character, dsrKey));
			}
		}
		break;
	case WM_PAINT:
		parent->queueInputEvent(new dsr::WindowEvent(dsr::WindowEventType::Redraw, parent->windowWidth, parent->windowHeight));
		// BeginPaint and EndPaint must be called with the given hwnd to prevent having the redraw message sent again
		parent->redraw(hwnd);
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
	// TODO: Keyboard presses & typing
	default:
		result = DefWindowProc(hwnd, message, wParam, lParam);
	}
	return result;
}

void Win32Window::prefetchEvents() {
	MSG messages;
	// Windows hangs unless we process application events for each window
	while (PeekMessage(&messages, NULL, 0, 0, PM_REMOVE)) {
		//dsr::printText("Received an event ", messages.message, "(", messages.wParam, ", ", (intptr_t)messages.lParam, ")\n");
		TranslateMessage(&messages);
		DispatchMessage(&messages); // Calling WindowProcedure for each window instance
	}
}

// Locked because it overrides
void Win32Window::resizeCanvas(int width, int height) {
	// Create a new canvas
	//   Even thou Windows is using RGBA pack order for the window, the bitmap format used for drawing is using BGRA order
	this->canvas = dsr::image_create_RgbaU8_native(width, height, dsr::PackOrderIndex::BGRA);
}
Win32Window::~Win32Window() {
	// Destroy the native window
	DestroyWindow(this->hwnd);
}

void Win32Window::redraw(HWND& hwnd) {
	// Let the source bitmap use a padded width to safely handle the stride
	// Windows require 8-byte alignment, but the image format uses 16-byte alignment.
	int paddedWidth = dsr::image_getStride(this->canvas) / 4;
	//int width = dsr::image_getWidth(this->canvas);
	int height = dsr::image_getHeight(this->canvas);
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
		SetDIBitsToDevice(targetContext, 0, 0, paddedWidth, height, 0, 0, 0, height, dsr::image_dangerous_getData(this->canvas), &bmi, DIB_RGB_COLORS);
	EndPaint(this->hwnd, &paintStruct);
}

void Win32Window::showCanvas() {
	this->prefetchEvents();
	this->redraw(this->hwnd);
}

std::shared_ptr<dsr::BackendWindow> createBackendWindow(const dsr::String& title, int width, int height) {
	auto backend = std::make_shared<Win32Window>(title, width, height);
	return std::dynamic_pointer_cast<dsr::BackendWindow>(backend);
}
