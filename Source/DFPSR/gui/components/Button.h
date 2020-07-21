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
	PersistentColor color;
	PersistentString text;
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
	bool isContainer() const;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	void receiveMouseEvent(const MouseEvent& event) override;
	bool pointIsInside(const IVector2D& pixelPosition) override;
	void changedTheme(VisualTheme newTheme) override;
	void changedLocation(IRect &oldLocation, IRect &newLocation) override;
	void changedAttribute(const ReadableString &name) override;
};

}

#endif

