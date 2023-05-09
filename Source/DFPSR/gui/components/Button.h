// zlib open source license
//
// Copyright (c) 2018 to 2019 David Forsgren Piuva
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

#ifndef DFPSR_GUI_COMPONENT_BUTTON
#define DFPSR_GUI_COMPONENT_BUTTON

#include "../VisualComponent.h"
#include "../../api/fontAPI.h"

namespace dsr {

class Button : public VisualComponent {
PERSISTENT_DECLARATION(Button)
public:
	// Attributes
	PersistentColor backColor = PersistentColor(130, 130, 130);
	PersistentColor foreColor = PersistentColor(0, 0, 0);
	PersistentString text;
	PersistentInteger padding = PersistentInteger(5); // How many pixels of padding are applied on each side of the text when calculating desired dimensions for placing in toolbars.
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
private:
	// Temporary
	bool pressed = false;
	bool inside = false;
	// Given from the style
	MediaMethod button;
	RasterFont font;
	void completeAssets();
	void generateGraphics();
	// Generated
	bool hasImages = false;
	OrderedImageRgbaU8 imageUp;
	OrderedImageRgbaU8 imageDown;
public:
	Button();
public:
	bool isContainer() const override;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	void receiveMouseEvent(const MouseEvent& event) override;
	bool pointIsInside(const IVector2D& pixelPosition) override;
	void changedTheme(VisualTheme newTheme) override;
	void changedLocation(const IRect &oldLocation, const IRect &newLocation) override;
	void changedAttribute(const ReadableString &name) override;
	IVector2D getDesiredDimensions() override;
};

}

#endif

