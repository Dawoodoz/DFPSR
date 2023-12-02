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

#ifndef DFPSR_GUI_COMPONENT_MENU
#define DFPSR_GUI_COMPONENT_MENU

#include "../VisualComponent.h"
#include "../../api/fontAPI.h"

namespace dsr {

class Menu : public VisualComponent {
PERSISTENT_DECLARATION(Menu)
public:
	// Attributes
	PersistentColor backColor = PersistentColor(130, 130, 130);
	PersistentColor foreColor = PersistentColor(0, 0, 0);
	// The text to display for the head that is directly interacted with.
	PersistentString text;
	// Name of theme class used to draw the background of the head.
	//   "MenuTop" is used if listClass is empty or not found, and the menu is not directly within another menu.
	//   "MenuSub" is used if listClass is empty or not found, and the menu is a direct child of another menu.
	PersistentString headClass;
	// Name of theme class used to draw the background of the drop-down list.
	//   "MenuList" is used if listClass is empty or not found.
	PersistentString listClass;
	PersistentInteger padding = PersistentInteger(4); // Empty space around child components and its own text.
	PersistentInteger spacing = PersistentInteger(2); // Empty space between child components.
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
private:
	void loadTheme(const VisualTheme &theme);
	void completeAssets();
	void generateGraphics();
	void generateBackground();
	MediaMethod headImageMethod, listBackgroundImageMethod;
	OrderedImageRgbaU8 headImage, listBackgroundImage;
	ComponentState headStateListenerMask; // Bit-mask telling which state bits are requested as input arguments to headImageMethod.
	RasterFont font;
	bool subMenu = false;
	IRect overlayLocation; // Relative to the parent's location, just like its own location
	bool pressed = false;
	// Settings fetched from the theme
	String finalHeadClass; // The selected HeadClass/Class from layout settings or the component's default theme class "MenuTop" or "MenuSub" based on ownership.
	String finalListClass; // The selected ListClass from layout settings or the component's default theme class "MenuList".
	int menuHead_filter = 0; // 0 for solid, 1 for alpha filter.
	int menuList_filter = 0; // 0 for solid, 1 for alpha filter.
public:
	Menu();
public:
	bool isContainer() const override;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	void drawOverlay(ImageRgbaU8& targetImage, const IVector2D &absoluteOffset) override;
	void changedTheme(VisualTheme newTheme) override;
	void changedLocation(const IRect &oldLocation, const IRect &newLocation) override;
	void changedAttribute(const ReadableString &name) override;
	void updateLocationEvent(const IRect& oldLocation, const IRect& newLocation) override;
	void receiveMouseEvent(const MouseEvent& event) override;
	IVector2D getDesiredDimensions() override;
	bool managesChildren() override;
public:
	// Helper functions for decorations.
	bool hasArrow();
	// Helper functions for overlay projecting components.
	void createOverlay();
	void updateStateEvent(ComponentState oldState, ComponentState newState) override;
	bool pointIsInsideOfOverlay(const IVector2D& pixelPosition) override;
};

}

#endif

