
#ifndef INPUT_TEST
#define INPUT_TEST

#include "../Test.h"

// buttonCount = 0 means that this test can not be performed at all.
// buttonCount = 1 means that the test will only require access to the left mouse button. (physically the right button if swapped for left handed users)
// buttonCount = 2 means that both left and right buttons are available, but not the middle button or clickable scroll wheel.
// buttonCount = 3 means that all three mouse buttons (left, right and middle) are available.
// While three buttons are available in the media layer, one should try to design applications to be useful even if only the left mouse button exists.
// The right button and scroll wheel can add convenience when available to the user.
// While more than three mouse buttons is not supported directly, they can still be bound as an extension to the keyboard using external settings.

// relative = true means that the used input device applies an offset to the cursor, and then the cursor's location can be reassigned to a new location without conflicts. (mouse, stick, ball, trackpad)
// relative = false means that the used input device sets an absolute cursor position from direct interaction with the screen, so that nothing else can move the cursor at the same time. (stylus pen, touch screen, eye-tracker)
// Touch screens are assumed to allow light touch for hovering and hard touch for clicking, because this is essential for using desktop applications. Otherwise hover effects will be stuck on the last clicked location.

// verticalScroll = true means that there exists a vertical scroll wheel or the two finger gesture to allow scrolling vertically. 
// verticalScroll = false is used to skip all tests that require vertical scrolling.
// Horizontal scrolling is currently not supported, because it mostly requires getting a laptop and the tests should be possible to complete with a three button mouse that can easily be moved around between computers.
void inputTests_populate(dsr::List<Test> &target, int buttonCount = 3, bool relative = true, bool verticalScroll = true);

#endif
