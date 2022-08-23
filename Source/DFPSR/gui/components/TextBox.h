// zlib open source license
//
// Copyright (c) 2022 David Forsgren Piuva
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

#ifndef DFPSR_GUI_COMPONENT_TEXTBOX
#define DFPSR_GUI_COMPONENT_TEXTBOX

#include "../VisualComponent.h"
#include "../../api/fontAPI.h"

namespace dsr {

class TextBox : public VisualComponent {
PERSISTENT_DECLARATION(TextBox)
public:
	// Attributes
	PersistentColor color;
	PersistentString text;
	void declareAttributes(StructureDefinition &target) const override;
	Persistent* findAttribute(const ReadableString &name) override;
	// Temporary
	bool mousePressed = false;
	uint32_t combinationKeys = 0;
	// Selection goes from selectionStart to beamLocation using bi-directional exclusive character indices.
	//   Empty with selectionStart = beamLocation.
	//   From the left with selectionStart < beamLocation.
	//   From the right with beamLocation < selectionStart.
	int64_t selectionStart = 0;
	int64_t beamLocation = 0;
	void limitSelection();
	void replaceSelection(const ReadableString replacingText);
	void replaceSelection(DsrChar replacingCharacter);
	void placeBeam(int64_t index, bool removeSelection);
private:
	// Given from the style
	MediaMethod textBox;
	RasterFont font;
	void completeAssets();
	void generateGraphics();
	// Generated
	bool hasImages = false;
	bool drawnAsFocused = false;
	OrderedImageRgbaU8 image;
public:
	TextBox();
public:
	bool isContainer() const override;
	void drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) override;
	void receiveMouseEvent(const MouseEvent& event) override;
	void receiveKeyboardEvent(const KeyboardEvent& event) override;
	bool pointIsInside(const IVector2D& pixelPosition) override;
	void changedTheme(VisualTheme newTheme) override;
	void changedLocation(const IRect &oldLocation, const IRect &newLocation) override;
	void changedAttribute(const ReadableString &name) override;
};

}

#endif

