﻿// zlib open source license
//
// Copyright (c) 2021 David Forsgren Piuva
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

#ifndef DFPSR_GUI_COMPONENT_PICTURE
#define DFPSR_GUI_COMPONENT_PICTURE

#include "../VisualComponent.h"

namespace dsr {

class Picture : public VisualComponent {
PERSISTENT_DECLARATION(Picture)
public:
	// Attributes
	PersistentImage image; // The default image
	PersistentImage imagePressed; // Only visible when pressing like a button (Requires clickable)
	PersistentBoolean interpolation; // False (0) for nearest neighbor, True (1) for bi-linear
	PersistentBoolean clickable; // Allow catching mouse events (false by default)
	// Generated
	bool hasImages = false;
	OrderedImageRgbaU8 finalImage, finalImagePressed;
	void generateGraphics();
	// Temporary
	bool pressed = false;
	bool inside = false;
	// Attribute access
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
public:
	Picture();
public:
	bool isContainer() const override;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	void receiveMouseEvent(const MouseEvent& event) override;
	bool pointIsInside(const IVector2D& pixelPosition) override;
	void changedLocation(const IRect &oldLocation, const IRect &newLocation) override;
	void changedAttribute(const ReadableString &name) override;
};

}

#endif
