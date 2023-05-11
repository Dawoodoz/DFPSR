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
	PersistentString text;
	PersistentInteger padding = PersistentInteger(4); // Empty space around child components and its own text.
	PersistentInteger spacing = PersistentInteger(2); // Empty space between child components.
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
private:
	void completeAssets();
	void generateGraphics();
	void generateBackground();
	MediaMethod headImageMethod, listBackgroundImageMethod;
	OrderedImageRgbaU8 headImage, listBackgroundImage;
	RasterFont font;
	bool subMenu = false;
	IRect overlayLocation; // Relative to the parent's location, just like its own location
	// Generated
	bool hasImages = false;
	OrderedImageRgbaU8 imageUp;
	OrderedImageRgbaU8 imageDown;
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

