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

#ifndef DFPSR_GUI_INPUT_EVENT
#define DFPSR_GUI_INPUT_EVENT

#include "../math/IVector.h"
#include <functional>

namespace dsr {

class InputEvent {
public:
	InputEvent() {}
	virtual ~InputEvent() {}
};

enum class KeyboardEventType { KeyDown, KeyUp, KeyType };

// The DsrKey enumeration is convertible to integers and allow certain well defined math operations
// Safe assumptions:
//   * DsrKey_0 to DsrKey_9 are guaranteed to be in an increasing serial order (so that "key - DsrKey_0" is the key's number)
//   * DsrKey_F1 to DsrKey_F12 are guaranteed to be in an increasing serial order (so that "key - (DsrKey_F1 - 1)" is the key's number)
//   * DsrKey_A to DsrKey_Z are guaranteed to be in an increasing serial order
// Characters are case insensitive, because DsrKey refers to the physical key.
//   Use the decoded Unicode value in DsrChar if you want to distinguish between upper and lower case or use special characters.
// Control, shift and alt combines left and right sides, because sometimes the system does not say if the key is left or right.
enum DsrKey {
	DsrKey_LeftArrow, DsrKey_RightArrow, DsrKey_UpArrow, DsrKey_DownArrow, DsrKey_PageUp, DsrKey_PageDown,
	DsrKey_Control, DsrKey_Shift, DsrKey_Alt, DsrKey_Escape, DsrKey_Pause, DsrKey_Space, DsrKey_Tab,
	DsrKey_Return, DsrKey_BackSpace, DsrKey_Delete, DsrKey_Insert, DsrKey_Home, DsrKey_End,
	DsrKey_0, DsrKey_1, DsrKey_2, DsrKey_3, DsrKey_4, DsrKey_5, DsrKey_6, DsrKey_7, DsrKey_8, DsrKey_9,
	DsrKey_F1, DsrKey_F2, DsrKey_F3, DsrKey_F4, DsrKey_F5, DsrKey_F6, DsrKey_F7, DsrKey_F8, DsrKey_F9, DsrKey_F10, DsrKey_F11, DsrKey_F12,
	DsrKey_A, DsrKey_B, DsrKey_C, DsrKey_D, DsrKey_E, DsrKey_F, DsrKey_G, DsrKey_H, DsrKey_I, DsrKey_J, DsrKey_K, DsrKey_L, DsrKey_M,
	DsrKey_N, DsrKey_O, DsrKey_P, DsrKey_Q, DsrKey_R, DsrKey_S, DsrKey_T, DsrKey_U, DsrKey_V, DsrKey_W, DsrKey_X, DsrKey_Y, DsrKey_Z,
	// TODO: Add any missing essential keys.
	DsrKey_Unhandled
};

class KeyboardEvent : public InputEvent {
public:
	// What the user did to the key.
	KeyboardEventType keyboardEventType;
	// The raw unicode value without any encoding.
	DsrChar character;
	// Minimal set of keys for portability.
	DsrKey dsrKey;
	KeyboardEvent(KeyboardEventType keyboardEventType, DsrChar character, DsrKey dsrKey)
	 : keyboardEventType(keyboardEventType), character(character), dsrKey(dsrKey) {}
};

enum class MouseKeyEnum { NoKey, Left, Right, Middle, ScrollUp, ScrollDown };
enum class MouseEventType { MouseDown, MouseUp, MouseMove, Scroll };
class MouseEvent : public InputEvent {
public:
	MouseEventType mouseEventType;
	MouseKeyEnum key;
	IVector2D position; // Pixel coordinates relative to upper left corner of parent container
	MouseEvent(MouseEventType mouseEventType, MouseKeyEnum key, IVector2D position)
	: mouseEventType(mouseEventType), key(key), position(position) {}
};
inline MouseEvent operator+(const MouseEvent &old, const IVector2D &offset) {
	MouseEvent result = old;
	result.position = result.position + offset;
	return result;
}
inline MouseEvent operator-(const MouseEvent &old, const IVector2D &offset) {
	MouseEvent result = old;
	result.position = result.position - offset;
	return result;
}
inline MouseEvent operator*(const MouseEvent &old, int scale) {
	MouseEvent result = old;
	result.position = result.position * scale;
	return result;
}
inline MouseEvent operator/(const MouseEvent &old, int scale) {
	MouseEvent result = old;
	result.position = result.position / scale;
	return result;
}

enum class WindowEventType { Close, Redraw };
class WindowEvent : public InputEvent {
public:
	WindowEventType windowEventType;
	int width, height;
	WindowEvent(WindowEventType windowEventType, int width, int height)
	: windowEventType(windowEventType), width(width), height(height) {}
};

// A macro for declaring a virtual callback from the base method.
//   Use the getter for registering methods so that they can be forwarded to a wrapper without inheritance.
//   Use the actual variable beginning with `callback_` when calling the method from inside.
#define DECLARE_CALLBACK(NAME, LAMBDA) \
	decltype(LAMBDA) callback_##NAME = LAMBDA; \
	decltype(LAMBDA)& NAME() { return callback_##NAME; }

// The callback types.
using EmptyCallback = std::function<void()>;
using IndexCallback = std::function<void(int index)>;
using SizeCallback = std::function<void(int width, int height)>;
using KeyboardCallback = std::function<void(const KeyboardEvent& event)>;
using MouseCallback = std::function<void(const MouseEvent& event)>;

// The default functions to call until a callback has been selected.
static EmptyCallback emptyCallback = []() {};
static IndexCallback indexCallback = [](int64_t index) {};
static SizeCallback sizeCallback = [](int width, int height) {};
static KeyboardCallback keyboardCallback = [](const KeyboardEvent& event) {};
static MouseCallback mouseCallback = [](const MouseEvent& event) {};

// Conversion to text for easy debugging.
String getName(DsrKey v);
String getName(KeyboardEventType v);
String getName(MouseKeyEnum v);
String getName(MouseEventType v);
String getName(WindowEventType v);
String& string_toStreamIndented(String& target, const DsrKey& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const KeyboardEventType& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const MouseKeyEnum& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const MouseEventType& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const WindowEventType& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const KeyboardEvent& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const MouseEvent& source, const ReadableString& indentation);
String& string_toStreamIndented(String& target, const WindowEvent& source, const ReadableString& indentation);

}

#endif
