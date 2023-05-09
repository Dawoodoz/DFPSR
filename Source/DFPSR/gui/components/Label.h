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

#ifndef DFPSR_GUI_COMPONENT_LABEL
#define DFPSR_GUI_COMPONENT_LABEL

#include "../VisualComponent.h"
#include "../../api/fontAPI.h"

namespace dsr {

class Label : public VisualComponent {
PERSISTENT_DECLARATION(Label)
public:
	// Attributes
	PersistentColor color = PersistentColor(0, 0, 0);
	// TODO: Why is "PersistentInteger opacity(255);" not recognizing the constructor?
	PersistentInteger opacity = PersistentInteger(255); // 0 is fully invisible, 255 is fully opaque
	PersistentString text;
	PersistentInteger padding = PersistentInteger(3); // How many pixels of padding are applied on each side of the text when calculating desired dimensions for placing in toolbars.
	// Attribute access
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
private:
	// Given from the style
	RasterFont font;
	void completeAssets();
public:
	Label();
public:
	bool isContainer() const override;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	bool pointIsInside(const IVector2D& pixelPosition) override;
	IVector2D getDesiredDimensions() override;
};

}

#endif

