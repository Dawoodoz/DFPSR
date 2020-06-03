// zlib open source license
//
// Copyright (c) 2020 David Forsgren Piuva
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

#ifndef DFPSR_GUI_COMPONENT_LISTBOX
#define DFPSR_GUI_COMPONENT_LISTBOX

#include "../VisualComponent.h"
#include "../Font.h"

namespace dsr {

class ListBox : public VisualComponent {
PERSISTENT_DECLARATION(ListBox)
public:
	// Attributes
	PersistentColor color;
	PersistentStringList list;
	PersistentInteger selectedIndex; // Should always be inside of the list's 0..length-1 bound or zero.
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
private:
	// Temporary
	bool holdingScrollBar = false;
	bool inside = false;
	bool hasVerticalScroll = false;
	int64_t pressedIndex = -1; // Index of pressed item or -1 for none.
	int64_t firstVisible = 0; // Index of first visible element for scrolling. May never go below zero.
	// Given from the style
	MediaMethod listBox;
	std::shared_ptr<RasterFont> font;
	void completeAssets();
	void generateGraphics();
	// Generated
	bool hasImages = false;
	OrderedImageRgbaU8 image;
	// Helper methods
	// Returns the selected index referring to an existing element or -1 if none is selected
	int64_t getSelectedIndex();
	void limitSelection(); // Clamp selection to valid range
	void limitScrolling(bool keepSelectedVisible = false); // Clamp scrolling
	int64_t getVisibleScrollRange(); // Return the number of items that are visible at once
	void pressScrollBar(int64_t localY); // Press the scroll-bar at localY in pixels
public:
	ListBox();
public:
	bool isContainer() const;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	void receiveMouseEvent(const MouseEvent& event) override;
	void changedTheme(VisualTheme newTheme) override;
	void changedLocation(IRect &oldLocation, IRect &newLocation) override;
	void changedAttribute(const ReadableString &name) override;
	// The call receiver decides if the input needs to be mangled into quotes
	String call(const ReadableString &methodName, const ReadableString &arguments) override;
};

}

#endif
