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

#include "TextBox.h"
#include <math.h>
#include <functional>

using namespace dsr;

PERSISTENT_DEFINITION(TextBox)

void TextBox::declareAttributes(StructureDefinition &target) const {
	VisualComponent::declareAttributes(target);
	target.declareAttribute(U"BackColor");
	target.declareAttribute(U"ForeColor");
	target.declareAttribute(U"Text");
	target.declareAttribute(U"MultiLine");
}

Persistent* TextBox::findAttribute(const ReadableString &name) {
	if (string_caseInsensitiveMatch(name, U"BackColor")) {
		return &(this->backColor);
	} else if (string_caseInsensitiveMatch(name, U"ForeColor")) {
		return &(this->foreColor);
	} else if (string_caseInsensitiveMatch(name, U"Text")) {
		return &(this->text);
	} else if (string_caseInsensitiveMatch(name, U"MultiLine")) {
		return &(this->multiLine);
	} else {
		return VisualComponent::findAttribute(name);
	}
}

TextBox::TextBox() {}

bool TextBox::isContainer() const {
	return false;
}

// Limit exclusive indices to the text.
void TextBox::limitSelection() {
	int64_t textLength = string_length(this->text.value);
	if (this->selectionStart < 0) this->selectionStart = 0;
	if (this->beamLocation < 0) this->beamLocation = 0;
	if (this->selectionStart > textLength) this->selectionStart = textLength;
	if (this->beamLocation > textLength) this->beamLocation = textLength;
}

static void tabJump(int64_t &x, int64_t leftOrigin, int64_t tabWidth) {
	x += tabWidth - ((x - leftOrigin) % tabWidth);
}

// To have a stable tab alignment, the whole text must be given when iterating.
void iterateCharacters(const ReadableString& text, const RasterFont &font, int64_t originX, std::function<void(int64_t index, DsrChar code, int64_t left, int64_t right)> characterAction) {
	int64_t right = originX;
	int64_t tabWidth = font_getTabWidth(font);
	int64_t monospaceWidth = font_getMonospaceWidth(font);
	for (int64_t i = 0; i <= string_length(text); i++) {
		DsrChar code = text[i];
		int64_t left = right;
		if (code == U'\t') {
			tabJump(right, originX, tabWidth);
		} else {
			right += monospaceWidth;
		}
		characterAction(i, code, left, right);
	}
}

// Iterate over the whole text once for both selection and characters.
// Returns the beam's X location in pixels relative to the parent of originX.
int64_t printMonospaceLine(OrderedImageRgbaU8 &target, const ReadableString& text, const RasterFont &font, ColorRgbaI32 foreColor, bool focused, int64_t originX, int64_t selectionLeft, int64_t selectionRight, int64_t beamIndex, int64_t topY, int64_t bottomY) {
	int64_t characterHeight = bottomY - topY;
	int64_t beamPixelX = originX;
	iterateCharacters(text, font, originX, [&target, &font, &foreColor, &beamPixelX, selectionLeft, selectionRight, beamIndex, topY, characterHeight, focused](int64_t index, DsrChar code, int64_t left, int64_t right) {
		if (index == beamIndex) beamPixelX = left;
		if (focused && selectionLeft <= index && index < selectionRight) {
			draw_rectangle(target, IRect(left, topY, right - left, characterHeight), ColorRgbaI32(0, 0, 100, 255));
			font_printCharacter(target, font, code, IVector2D(left, topY), ColorRgbaI32(255, 255, 255, 255));
		} else {
			font_printCharacter(target, font, code, IVector2D(left, topY), foreColor);
		}
	});
	return beamPixelX;
}

void TextBox::updateLines() {
	// Index the lines for fast scrolling and rendering.
	this->lines.clear();
	int64_t sectionStart = 0;
	int64_t textLength = string_length(this->text.value);
	for (int64_t i = 0; i < textLength; i++) {
		if (this->text.value[i] == U'\n') {
			this->lines.pushConstruct(sectionStart, i);
			sectionStart = i + 1;
		}
	}
	// Always include the line after a linebreak, even if it is empty.
	this->lines.pushConstruct(sectionStart, textLength);
}

LVector2D TextBox::getTextOrigin() {
	int64_t rowStride = font_getSize(this->font);
	int64_t halfRowStride = rowStride / 2;
	int64_t firstVisibleLine = this->verticalScroll / rowStride;
	return LVector2D(halfRowStride - this->horizontalScroll, this->multiLine.value ? (halfRowStride + (firstVisibleLine * rowStride) - this->verticalScroll) : ((image_getHeight(this->image) / 2) - halfRowStride));
}

// TODO: Reuse scaled background images as a separate layer.
// TODO: Allow using different colors for beam, selection, selected text, normal text...
//       Maybe ask a separate color palette for specific things using the specific class of textboxes.
//       Color palettes can be independent of the media machine, allowing them to be mixed freely with different themes.
//       Color palettes can be loaded together with the layout to instantly have the requested standard colors by name.
//       Color palettes can have a standard column order of input to easily pack multiple color themes into the same color palette image.
//         Just a long list of names for the different X coordinates and the user selects a Y coordinate as the color theme.
//         New components will have to use existing parts of the palette by keeping the names reusable.
//       Separate components should be able to override any color for programmability, but default values should refer to the current color palette.
//         If no color is assigned, the class will give it a standard color from the theme.
//         Should classes be separate for themes and palettes?
void TextBox::generateGraphics() {
	int width = this->location.width();
	int height = this->location.height();
	if (width < 1) { width = 1; }
	if (height < 1) { height = 1; }
	bool focused = this->isFocused();
	if (!this->indexedLines) {
		this->updateLines();
		this->indexedLines = true;
	}
	if (!this->hasImages || this->drawnAsFocused != focused) {
		this->hasImages = true;
		this->drawnAsFocused = focused;
		completeAssets();
		ColorRgbaI32 backColor = ColorRgbaI32(this->backColor.value, 255);
		ColorRgbaI32 foreColor = ColorRgbaI32(this->foreColor.value, 255);

		// Create a scaled image
		this->generateImage(this->textBox, width, height, backColor.red, backColor.green, backColor.blue, 0, focused ? 1 : 0)(this->image);
		this->limitSelection();
		LVector2D origin = this->getTextOrigin();
		int64_t rowStride = font_getSize(this->font);
		int64_t targetHeight = image_getHeight(this->image);
		int64_t firstVisibleLine = this->verticalScroll / rowStride;

		// Find character indices for left and right sides.
		int64_t selectionLeft = std::min(this->selectionStart, this->beamLocation);
		int64_t selectionRight = std::max(this->selectionStart, this->beamLocation);
		bool hasSelection = selectionLeft < selectionRight;

		// Draw the text with selection and get the beam's pixel location.
		int64_t topY = origin.y;
		for (int row = firstVisibleLine; row < this->lines.length() && topY < targetHeight; row++) {
			int64_t startIndex = this->lines[row].lineStartIndex;
			int64_t endIndex = this->lines[row].lineEndIndex;
			ReadableString currentLine = string_exclusiveRange(this->text.value, startIndex, endIndex);
			int64_t beamPixelX = printMonospaceLine(this->image, currentLine, this->font, foreColor, focused, origin.x, selectionLeft - startIndex, selectionRight - startIndex, this->beamLocation - startIndex, topY, topY + rowStride);
			// Draw a beam if the textbox is focused.
			if (focused && this->beamLocation >= startIndex && this->beamLocation <= endIndex) {
				int64_t beamWidth = 2;
				draw_rectangle(this->image, IRect(beamPixelX - 1, topY - 1, beamWidth, rowStride + 2), hasSelection ? ColorRgbaI32(255, 255, 255, 255) : foreColor);
			}
			topY += rowStride;
		}
	}
}

void TextBox::drawSelf(ImageRgbaU8& targetImage, const IRect &relativeLocation) {
	this->generateGraphics();
	draw_copy(targetImage, this->image, relativeLocation.left(), relativeLocation.top());
}

int64_t TextBox::findBeamLocationInLine(int64_t rowIndex, int64_t pixelX) {
	LVector2D origin = this->getTextOrigin();
	// Clamp to the closest row if going outside.
	if (rowIndex < 0) rowIndex = 0;
	if (rowIndex >= this->lines.length()) rowIndex = this->lines.length() - 1;
	int64_t beamIndex = 0;
	int64_t closestDistance = 1000000000000;
	int64_t startIndex = this->lines[rowIndex].lineStartIndex;
	int64_t endIndex = this->lines[rowIndex].lineEndIndex;
	ReadableString currentLine = string_exclusiveRange(this->text.value, startIndex, endIndex);
	iterateCharacters(currentLine, font, origin.x, [&beamIndex, &closestDistance, pixelX](int64_t index, DsrChar code, int64_t left, int64_t right) {
		int64_t center = (left + right) / 2;
		int64_t newDistance = std::abs(pixelX - center);
		if (newDistance < closestDistance) {
			beamIndex = index;
			closestDistance = newDistance;
		}
	});
	return startIndex + beamIndex;
}

int64_t TextBox::findBeamLocation(const LVector2D &pixelLocation) {
	LVector2D origin = this->getTextOrigin();
	int64_t rowStride = font_getSize(this->font);
	int64_t rowIndex = (pixelLocation.y - origin.y) / rowStride;
	return this->findBeamLocationInLine(rowIndex, pixelLocation.x);
}

void TextBox::receiveMouseEvent(const MouseEvent& event) {
	if (event.mouseEventType == MouseEventType::MouseDown) {
		this->mousePressed = true;
		int64_t newBeamIndex = findBeamLocation(LVector2D(event.position.x - this->location.left(), event.position.y - this->location.top()));
		if (newBeamIndex != this->selectionStart || newBeamIndex != this->beamLocation) {
			this->selectionStart = newBeamIndex;
			this->beamLocation = newBeamIndex;
			this->hasImages = false;
		}
	} else if (this->mousePressed && event.mouseEventType == MouseEventType::MouseMove) {
		if (this->mousePressed) {
			int64_t newBeamIndex = findBeamLocation(LVector2D(event.position.x - this->location.left(), event.position.y - this->location.top()));
			if (newBeamIndex != this->beamLocation) {
				this->beamLocation = newBeamIndex;
				this->hasImages = false;
			}
		}
	} else if (this->mousePressed && event.mouseEventType == MouseEventType::MouseUp) {
		this->mousePressed = false;
	}
	VisualComponent::receiveMouseEvent(event);
}

void TextBox::replaceSelection(const ReadableString replacingText) {
	int64_t selectionLeft = std::min(this->selectionStart, this->beamLocation);
	int64_t selectionRight = std::max(this->selectionStart, this->beamLocation);
	this->text.value = string_combine(string_before(this->text.value, selectionLeft), replacingText, string_from(this->text.value, selectionRight));
	// Place beam on the right side of the replacement without selecting anything
	this->selectionStart = selectionLeft + string_length(replacingText);
	this->beamLocation = selectionStart;
	this->indexedLines = false;
	this->hasImages = false;
}

void TextBox::replaceSelection(DsrChar replacingCharacter) {
	int64_t selectionLeft = std::min(this->selectionStart, this->beamLocation);
	int64_t selectionRight = std::max(this->selectionStart, this->beamLocation);
	String newText = string_before(this->text.value, selectionLeft);
	string_appendChar(newText, replacingCharacter);
	string_append(newText, string_from(this->text.value, selectionRight));
	this->text.value = newText;
	// Place beam on the right side of the replacement without selecting anything
	this->selectionStart = selectionLeft + 1;
	this->beamLocation = selectionStart;
	this->indexedLines = false;
	this->hasImages = false;
}

void TextBox::placeBeamAtCharacter(int64_t characterIndex, bool removeSelection) {
	this->beamLocation = characterIndex;
	if (removeSelection) {
		this->selectionStart = characterIndex;
	}
	this->hasImages = false;
}

void TextBox::moveBeamVertically(int64_t rowIndexOffset, bool removeSelection) {
	// Find the current beam's row index.
	int64_t oldRowIndex = 0;
	for (int row = 0; row < this->lines.length(); row++) {
		int64_t startIndex = this->lines[row].lineStartIndex;
		int64_t endIndex = this->lines[row].lineEndIndex;
		if (this->beamLocation >= startIndex && this->beamLocation <= endIndex) {
			oldRowIndex = row;
		}
	}
	// Find another row.
	int64_t newRowIndex = oldRowIndex + rowIndexOffset;
	if (newRowIndex < 0) { newRowIndex = 0; }
	if (newRowIndex >= this->lines.length()) { newRowIndex = this->lines.length() - 1; }
	// Get the old pixel offset from the beam.
	LVector2D origin = this->getTextOrigin();
	int64_t beamPixelX = 0;
	int64_t lineStartIndex = this->lines[oldRowIndex].lineStartIndex;
	int64_t lineEndIndex = this->lines[oldRowIndex].lineEndIndex;
	int64_t localBeamIndex = this->beamLocation - lineStartIndex;
	ReadableString currentLine = string_exclusiveRange(this->text.value, lineStartIndex, lineEndIndex);
	iterateCharacters(currentLine, font, origin.x, [&beamPixelX, localBeamIndex](int64_t index, DsrChar code, int64_t left, int64_t right) {
		if (index == localBeamIndex) beamPixelX = left;
	});
	printText(U"beamPixelX = ", beamPixelX, U"\n");
	// Get the closest location in the new row.
	int64_t newCharacterIndex = findBeamLocationInLine(newRowIndex, beamPixelX);
	printText(U"newCharacterIndex = ", newCharacterIndex, U"\n");
	placeBeamAtCharacter(newCharacterIndex, removeSelection);
}

static const uint32_t combinationKey_leftShift = 1 << 0;
static const uint32_t combinationKey_rightShift = 1 << 1;
static const uint32_t combinationKey_shift = combinationKey_leftShift | combinationKey_rightShift;
static const uint32_t combinationKey_leftControl = 1 << 2;
static const uint32_t combinationKey_rightControl = 1 << 3;
static const uint32_t combinationKey_control = combinationKey_leftControl | combinationKey_rightControl;

static int64_t getLineStart(const ReadableString &text, int64_t searchStart) {
	for (int64_t i = searchStart - 1; i >= 0; i--) {
		if (text[i] == U'\n') {
			return i + 1;
		}
	}
	return 0;
}

static int64_t getLineEnd(const ReadableString &text, int64_t searchStart) {
	for (int64_t i = searchStart; i < string_length(text); i++) {
		if (text[i] == U'\n') {
			return i;
		}
	}
	return string_length(text);
}

// TODO: Copy and paste using a clipboard. (With automatic removal of new lines when multi-line is disabled)
void TextBox::receiveKeyboardEvent(const KeyboardEvent& event) {
	// Insert and scroll-lock is not supported.
	if (event.keyboardEventType == KeyboardEventType::KeyDown) {
		if (event.dsrKey == DsrKey_LeftShift) {
			this->combinationKeys |= combinationKey_leftShift;
		} else if (event.dsrKey == DsrKey_RightShift) {
			this->combinationKeys |= combinationKey_rightShift;
		} else if (event.dsrKey == DsrKey_LeftControl) {
			this->combinationKeys |= combinationKey_leftControl;
		} else if (event.dsrKey == DsrKey_RightControl) {
			this->combinationKeys |= combinationKey_rightControl;
		}
	} else if (event.keyboardEventType == KeyboardEventType::KeyUp) {
		if (event.dsrKey == DsrKey_LeftShift) {
			this->combinationKeys &= ~combinationKey_leftShift;
		} else if (event.dsrKey == DsrKey_RightShift) {
			this->combinationKeys &= ~combinationKey_rightShift;
		} else if (event.dsrKey == DsrKey_LeftControl) {
			this->combinationKeys &= ~combinationKey_leftControl;
		} else if (event.dsrKey == DsrKey_RightControl) {
			this->combinationKeys &= ~combinationKey_rightControl;
		}
	} else if (event.keyboardEventType == KeyboardEventType::KeyType) {
		int64_t textLength = string_length(this->text.value);
		bool selected = this->selectionStart != this->beamLocation;
		bool printable = event.character == U'\t' || (31 < event.character && event.character < 127) || 159 < event.character;
		bool canGoLeft = textLength > 0 && this->beamLocation > 0;
		bool canGoRight = textLength > 0 && this->beamLocation < textLength;
		bool holdShift = this->combinationKeys & combinationKey_shift;
		bool holdControl = this->combinationKeys & combinationKey_control;
		bool removeSelection = !holdShift;
		if (selected && (event.dsrKey == DsrKey_BackSpace || event.dsrKey == DsrKey_Delete)) {
			// Remove selection
			this->replaceSelection(U"");
		} else if (event.dsrKey == DsrKey_BackSpace && canGoLeft) {
			// Erase left of beam
			this->beamLocation--;
			this->replaceSelection(U"");
		} else if (event.dsrKey == DsrKey_Delete && canGoRight) {
			// Erase right of beam
			this->beamLocation++;
			this->replaceSelection(U"");
		} else if (event.dsrKey == DsrKey_Home || (event.dsrKey == DsrKey_LeftArrow && holdControl)) {
			// Move to the line start using Home or Ctrl + LeftArrow
			this->placeBeamAtCharacter(getLineStart(this->text.value, this->beamLocation), removeSelection);
		} else if (event.dsrKey == DsrKey_End || (event.dsrKey == DsrKey_RightArrow && holdControl)) {
			// Move to the line end using End or Ctrl + RightArrow
			this->placeBeamAtCharacter(getLineEnd(this->text.value, this->beamLocation), removeSelection);
		} else if (event.dsrKey == DsrKey_LeftArrow && canGoLeft) {
			// Move left using LeftArrow
			this->placeBeamAtCharacter(this->beamLocation - 1, removeSelection);
		} else if (event.dsrKey == DsrKey_RightArrow && canGoRight) {
			// Move right using RightArrow
			this->placeBeamAtCharacter(this->beamLocation + 1, removeSelection);
		} else if (event.dsrKey == DsrKey_UpArrow) {
			// Move up using UpArrow
			this->moveBeamVertically(-1, removeSelection);
		} else if (event.dsrKey == DsrKey_DownArrow) {
			// Move down using DownArrow
			this->moveBeamVertically(1, removeSelection);
		} else if (event.dsrKey == DsrKey_Return) {
			if (this->multiLine.value) {
				this->replaceSelection(U'\n');
			}
		} else if (printable) {
			this->replaceSelection(event.character);
		}
		//printText(U"KeyType char=", event.character, " key=", event.dsrKey, U"\n");
	}
	VisualComponent::receiveKeyboardEvent(event);
}

bool TextBox::pointIsInside(const IVector2D& pixelPosition) {
	this->generateGraphics();
	// Get the point relative to the component instead of its direct container
	IVector2D localPoint = pixelPosition - this->location.upperLeft();
	// Sample opacity at the location
	return dsr::image_readPixel_border(this->image, localPoint.x, localPoint.y).alpha > 127;
}

void TextBox::changedTheme(VisualTheme newTheme) {
	this->textBox = theme_getScalableImage(newTheme, U"TextBox");
	this->hasImages = false;
}

void TextBox::completeAssets() {
	if (this->textBox.methodIndex == -1) {
		this->textBox = theme_getScalableImage(theme_getDefault(), U"TextBox");
	}
	if (this->font.get() == nullptr) {
		this->font = font_getDefault();
	}
}

void TextBox::changedLocation(const IRect &oldLocation, const IRect &newLocation) {
	// If the component has changed dimensions then redraw the image
	if (oldLocation.size() != newLocation.size()) {
		this->hasImages = false;
	}
}

void TextBox::changedAttribute(const ReadableString &name) {
	if (!string_caseInsensitiveMatch(name, U"Visible")) {
		this->hasImages = false;
		if (string_caseInsensitiveMatch(name, U"Text")) {
			this->limitSelection();
			this->indexedLines = false;
		}
	}
}
